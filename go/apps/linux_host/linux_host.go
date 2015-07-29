// Copyright (c) 2014, Kevin Walsh.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"net"
	"os"
	"os/exec"
	"os/signal"
	"path"
	"syscall"
	"text/tabwriter"

	"github.com/golang/glog"
	"github.com/golang/protobuf/proto"
	"github.com/jlmucb/cloudproxy/go/tao"
	"github.com/jlmucb/cloudproxy/go/util"
	"github.com/jlmucb/cloudproxy/go/util/options"
	"golang.org/x/crypto/ssh/terminal"
)

var opts = []options.Option{
	// Flags for all/most commands
	{"tao_domain", "", "<dir>", "Tao domain configuration directory", "all"},
	{"host", "", "<dir>", "Host configuration, relative to domain directory or absolute", "all"},
	{"quiet", false, "", "Be more quiet", "all"},

	// Flags for init (and start) command
	{"root", false, "", "Create a root host, not backed by any parent Tao", "init,start"},
	{"stacked", false, "", "Create a stacked host, backed by a parent Tao", "init,start"},
	// TODO(kwalsh) hosted program type should be selectable at time of
	// tao_launch. A single host should be able to host all types concurrently.
	{"hosting", "", "<type>", "Hosted program type: process, docker, or kvm_coreos", "init"},
	{"socket_dir", "", "<dir>", "Hosted program socket directory, relative to host directory or absolute", "init"},

	// Flags for start command
	{"foreground", false, "", "Run in the foreground", "start"},
	// Using setsid (1) and shell redirection is an alternative -daemon:
	//    sh$ setsid tao host start ... </dev/null >/dev/null 2>&1
	//    sh$ setsid linux_host start ... </dev/null >/dev/null 2>&1
	{"daemon", false, "", "Detach from tty, close stdio, and run as a daemon", "start"},

	// Flags for root
	{"pass", "", "<password>", "Host password for root hosts (for testing only!)", "root"},

	// Flags for stacked
	{"parent_type", "", "<type>", "Type of channel to parent Tao: TPM, pipe, file, or unix", "stacked"},
	{"parent_spec", "", "<spec>", "Spec for channel to parent Tao", "stacked"},

	// Flags for QEMU/KVM CoreOS init
	{"kvm_coreos_img", "", "<path>", "Path to CoreOS.img file, relative to domain or absolute", "kvm"},
	{"kvm_coreos_vm_memory", 0, "SIZE", "The amount of RAM (in KB) to give VM", "kvm"},
	// TODO(kwalsh) shouldn't keys be generated randomly within the host?
	// Otherwise, we need to trust whoever holds the keys, no?
	{"kvm_coreos_ssh_auth_keys", "", "<path>", "An authorized_keys file for SSH to CoreOS guest, relative to domain or absolute", "kvm"},
}

func init() {
	options.Add(opts...)
}

func help() {
	w := new(tabwriter.Writer)
	w.Init(os.Stderr, 4, 0, 2, ' ', 0)
	av0 := path.Base(os.Args[0])

	fmt.Fprintf(w, "Linux Tao Host\n")
	fmt.Fprintf(w, "Usage:\n")
	fmt.Fprintf(w, "  %s init [options]\t Initialize a new host\n", av0)
	fmt.Fprintf(w, "  %s show [options]\t Show host principal name\n", av0)
	fmt.Fprintf(w, "  %s start [options]\t Start the host\n", av0)
	fmt.Fprintf(w, "  %s stop [options]\t Request the host stop\n", av0)
	fmt.Fprintf(w, "\n")

	categories := []options.Category{
		{"all", "Basic options for most commands"},
		{"init", "Options for 'init' command"},
		{"start", "Options for 'start' command"},
		{"root", "Options for root hosts"},
		{"stacked", "Options for stacked hosts"},
		{"kvm", "Options for hosting QEMU/KVM CoreOS"},
		{"logging", "Options to control log output"},
	}
	options.ShowRelevant(w, categories...)

	w.Flush()
}

var noise = ioutil.Discard

func main() {
	flag.Usage = help

	// Get options before the command verb
	flag.Parse()
	// Get command verb
	cmd := "help"
	if flag.NArg() > 0 {
		cmd = flag.Arg(0)
	}
	// Get options after the command verb
	if flag.NArg() > 1 {
		flag.CommandLine.Parse(flag.Args()[1:])
	}

	if !*options.Bool["quiet"] {
		noise = os.Stdout
	}

	// Load the domain.
	domain, err := tao.LoadDomain(domainConfigPath(), nil)
	failIf(err, "Can't load domain")
	glog.Info("Domain guard: ", domain.Guard)

	switch cmd {
	case "help":
		help()
	case "init":
		initHost(domain)
	case "show":
		showHost(domain)
	case "start":
		startHost(domain)
	case "stop", "shutdown":
		stopHost(domain)
	default:
		usage("Unrecognized command: %s", cmd)
	}
}

func domainPath() string {
	if path := *options.String["tao_domain"]; path != "" {
		return path
	}
	if path := os.Getenv("TAO_DOMAIN"); path != "" {
		return path
	}
	usage("Must supply -tao_domain or set $TAO_DOMAIN")
	return ""
}

func domainConfigPath() string {
	return path.Join(domainPath(), "tao.config")
}

func hostPath() string {
	hostPath := *options.String["host"]
	if hostPath == "" {
		// usage("Must supply a -host path")
		hostPath = "linux_tao_host"
	}
	if !path.IsAbs(hostPath) {
		hostPath = path.Join(domainPath(), hostPath)
	}
	return hostPath
}

func hostConfigPath() string {
	return path.Join(hostPath(), "host.config")
}

// Update configuration based on command-line options. Does very little sanity checking.
func configureFromOptions(cfg *tao.LinuxHostConfig) {
	if *options.Bool["root"] && *options.Bool["stacked"] {
		usage("Can supply only one of -root and -stacked")
	} else if *options.Bool["root"] {
		cfg.Type = proto.String("root")
	} else if *options.Bool["stacked"] {
		cfg.Type = proto.String("stacked")
	} else if cfg.Type == nil {
		usage("Must supply one of -root and -stacked")
	}
	if s := *options.String["hosting"]; s != "" {
		cfg.Hosting = proto.String(s)
	}
	if s := *options.String["parent_type"]; s != "" {
		cfg.ParentType = proto.String(s)
	}
	if s := *options.String["parent_spec"]; s != "" {
		cfg.ParentSpec = proto.String(s)
	}
	if s := *options.String["socket_dir"]; s != "" {
		cfg.SocketDir = proto.String(s)
	}
	if s := *options.String["kvm_coreos_img"]; s != "" {
		cfg.KvmCoreosImg = proto.String(s)
	}
	if i := *options.Int["kvm_coreos_vm_memory"]; i != 0 {
		cfg.KvmCoreosVmMemory = proto.Int32(int32(i))
	}
	if s := *options.String["kvm_coreos_ssh_auth_keys"]; s != "" {
		cfg.KvmCoreosSshAuthKeys = proto.String(s)
	}
}

func configureFromFile() *tao.LinuxHostConfig {
	d, err := ioutil.ReadFile(hostConfigPath())
	if err != nil {
		fail(err, "Can't read linux host configuration")
	}
	var cfg tao.LinuxHostConfig
	if err := proto.UnmarshalText(string(d), &cfg); err != nil {
		fail(err, "Can't parse linux host configuration")
	}
	return &cfg
}

func loadHost(domain *tao.Domain, cfg *tao.LinuxHostConfig) *tao.LinuxHost {
	var tc tao.Config

	// Decide host type
	switch cfg.GetType() {
	case "root":
		tc.HostType = tao.Root
	case "stacked":
		tc.HostType = tao.Stacked
	case "":
		usage("Must supply -hosting flag")
	default:
		usage("Invalid host type: %s", cfg.GetType())
	}

	// Decide hosting type
	switch cfg.GetHosting() {
	case "process":
		tc.HostedType = tao.ProcessPipe
	case "docker":
		tc.HostedType = tao.DockerUnix
	case "kvm_coreos":
		tc.HostedType = tao.KVMCoreOSFile
	case "":
		usage("Must supply -hosting flag")
	default:
		usage("Invalid hosting type: %s", cfg.GetHosting())
	}

	// For stacked hosts, figure out the channel type: TPM, pipe, file, or unix
	if tc.HostType == tao.Stacked {
		switch cfg.GetParentType() {
		case "TPM":
			tc.HostChannelType = tao.TPM
		case "pipe":
			tc.HostChannelType = tao.Pipe
		case "file":
			tc.HostChannelType = tao.File
		case "unix":
			tc.HostChannelType = tao.Unix
		case "":
			usage("Must supply -parent_type for stacked hosts")
		default:
			usage("Invalid parent type: %s", cfg.GetParentType())
		}

		// For stacked hosts on anything but a TPM, we also need parent spec
		if tc.HostChannelType != tao.TPM {
			tc.HostSpec = cfg.GetParentSpec()
			if tc.HostSpec == "" {
				usage("Must supply -parent_spec for non-TPM stacked hosts")
			}
		} else {
			// For stacked hosts on a TPM, we also need info from domain config
			if domain.Config.TpmInfo == nil {
				usage("Must provide TPM configuration in the domain to use a TPM")
			}
			tc.TPMAIKPath = path.Join(domainPath(), domain.Config.TpmInfo.GetAikPath())
			tc.TPMPCRs = domain.Config.TpmInfo.GetPcrs()
			tc.TPMDevice = domain.Config.TpmInfo.GetTpmPath()
		}
	}

	rulesPath := ""
	if p := domain.RulesPath(); p != "" {
		rulesPath = path.Join(domainPath(), p)
	}

	// Create the hosted program factory
	socketPath := hostPath()
	if subPath := cfg.GetSocketDir(); subPath != "" {
		if path.IsAbs(subPath) {
			socketPath = subPath
		} else {
			socketPath = path.Join(socketPath, subPath)
		}
	}

	// TODO(cjpatton) How do the NewLinuxDockerContainterFactory and the
	// NewLinuxKVMCoreOSFactory need to be modified to support the new
	// CachedGuard? They probably don't.
	var childFactory tao.HostedProgramFactory
	switch tc.HostedType {
	case tao.ProcessPipe:
		childFactory = tao.NewLinuxProcessFactory("pipe", socketPath)
	case tao.DockerUnix:
		childFactory = tao.NewLinuxDockerContainerFactory(socketPath, rulesPath)
	case tao.KVMCoreOSFile:
		sshFile := cfg.GetKvmCoreosSshAuthKeys()
		if sshFile == "" {
			usage("Must specify -kvm_coreos_ssh_auth_keys for hosting QEMU/KVM CoreOS")
		}
		if !path.IsAbs(sshFile) {
			sshFile = path.Join(domainPath(), sshFile)
		}
		sshKeysCfg, err := tao.CloudConfigFromSSHKeys(sshFile)
		failIf(err, "Can't read ssh keys")

		coreOSImage := cfg.GetKvmCoreosImg()
		if coreOSImage == "" {
			usage("Must specify -kvm_coreos_image for hosting QEMU/KVM CoreOS")
		}
		if !path.IsAbs(coreOSImage) {
			coreOSImage = path.Join(domainPath(), coreOSImage)
		}

		vmMemory := cfg.GetKvmCoreosVmMemory()
		if vmMemory == 0 {
			vmMemory = 1024
		}

		cfg := &tao.CoreOSConfig{
			ImageFile:  coreOSImage,
			Memory:     int(vmMemory),
			RulesPath:  rulesPath,
			SSHKeysCfg: sshKeysCfg,
		}
		childFactory, err = tao.NewLinuxKVMCoreOSFactory(socketPath, cfg)
		failIf(err, "Can't create KVM CoreOS factory")
	}

	var host *tao.LinuxHost
	var err error
	switch tc.HostType {
	case tao.Root:
		pwd := getKey("root host key password", "pass")
		host, err = tao.NewRootLinuxHost(hostPath(), domain.Guard, pwd, childFactory)
	case tao.Stacked:
		parent := tao.ParentFromConfig(tc)
		if parent == nil {
			usage("No host tao available, verify -parent_type or $%s\n", tao.HostChannelTypeEnvVar)
		}
		host, err = tao.NewStackedLinuxHost(hostPath(), domain.Guard, tao.ParentFromConfig(tc), childFactory)
	}
	failIf(err, "Can't create host")

	return host
}

func initHost(domain *tao.Domain) {
	var cfg tao.LinuxHostConfig

	configureFromOptions(&cfg)
	_ = loadHost(domain, &cfg)

	// If we get here, keys were created and flags must be ok.

	file, err := util.CreatePath(hostConfigPath(), 0777, 0666)
	failIf(err, "Can't create host configuration")
	cs := proto.MarshalTextString(&cfg)
	fmt.Fprint(file, cs)
	file.Close()
}

func showHost(domain *tao.Domain) {
	cfg := configureFromFile()
	configureFromOptions(cfg)
	host := loadHost(domain, cfg)
	fmt.Printf("%v\n", host.HostName())
}

func isBoolFlagSet(name string) bool {
	f := flag.Lookup("logtostderr")
	if f == nil {
		return false
	}
	v, ok := f.Value.(flag.Getter).Get().(bool)
	return ok && v
}

func daemonize() {
	// For our purposes, "daemon" means being a session leader.
	sid, _, errno := syscall.Syscall(syscall.SYS_GETSID, 0, 0, 0)
	var err error
	if errno != 0 {
		err = errno
	}
	failIf(err, "Can't get process SID")
	if int(sid) != syscall.Getpid() {
		if syscall.Getpid() == syscall.Getpid() {
			fmt.Fprintf(noise, "Forking to enable setsid\n")
		} else {
			fmt.Fprintf(noise, "Forking anyway\n")
		}
		// No daemonize, but we can just fork/exec and exit
		path, err := os.Readlink("/proc/self/exe")
		failIf(err, "Can't get path to self executable")
		// special case: keep stderr if -logtostderr or -alsologtostderr
		stderr := os.Stderr
		if !isBoolFlagSet("logtostderr") && !isBoolFlagSet("alsologtostderr") {
			stderr = nil
		}
		spa := &syscall.SysProcAttr{
			Setsid: true, // Create session.
			// Setpgid: true, // Set process group ID to new pid (SYSV setpgrp)
			// Setctty: true, // Set controlling terminal to fd Ctty (only meaningful if Setsid is set)
			// Noctty: true, // Detach fd 0 from controlling terminal
			// Ctty: 0, // Controlling TTY fd (Linux only)
		}
		daemon := exec.Cmd{
			Path:        path,
			Args:        os.Args,
			Stderr:      stderr,
			SysProcAttr: spa,
		}
		err = daemon.Start()
		failIf(err, "Can't become daemon")
		fmt.Fprintf(noise, "Linux Tao Host running as daemon\n")
		os.Exit(0)
	} else {
		fmt.Fprintf(noise, "Already a session leader?\n")
	}
}

func startHost(domain *tao.Domain) {

	if *options.Bool["daemon"] && *options.Bool["foreground"] {
		usage("Can supply only one of -daemon and -foreground")
	}
	if *options.Bool["daemon"] {
		daemonize()
	}

	cfg := configureFromFile()
	configureFromOptions(cfg)
	host := loadHost(domain, cfg)

	sockPath := path.Join(hostPath(), "admin_socket")
	// Make sure callers can read the admin socket directory
	err := os.Chmod(path.Dir(sockPath), 0755)
	failIf(err, "Can't change permissions")
	uaddr, err := net.ResolveUnixAddr("unix", sockPath)
	failIf(err, "Can't resolve unix socket")
	sock, err := net.ListenUnix("unix", uaddr)
	failIf(err, "Can't create admin socket")
	defer sock.Close()
	err = os.Chmod(sockPath, 0666)
	if err != nil {
		sock.Close()
		fail(err, "Can't change permissions on admin socket")
	}

	go func() {
		fmt.Fprintf(noise, "Linux Tao Service (%s) started and waiting for requests\n", host.HostName())
		err = tao.NewLinuxHostAdminServer(host).Serve(sock)
		fmt.Fprintf(noise, "Linux Tao Service finished\n")
		sock.Close()
		failIf(err, "Error serving admin requests")
		os.Exit(0)
	}()

	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt, os.Kill, syscall.SIGTERM)
	<-c
	fmt.Fprintf(noise, "Linux Tao Service shutting down\n")
	err = shutdown()
	if err != nil {
		sock.Close()
		fail(err, "Can't shut down admin socket")
	}

	// The above goroutine will normally end by calling os.Exit(), so we
	// can block here indefinitely. But if we get a second kill signal,
	// let's abort.
	fmt.Fprintf(noise, "Waiting for shutdown....\n")
	<-c
	fail(nil, "Could not shut down linux_host")
}

func stopHost(domain *tao.Domain) {
	err := shutdown()
	if err != nil {
		usage("Couldn't connect to linux_host: %s", err)
	}
}

func shutdown() error {
	sockPath := path.Join(hostPath(), "admin_socket")
	conn, err := net.DialUnix("unix", nil, &net.UnixAddr{sockPath, "unix"})
	if err != nil {
		return err
	}
	defer conn.Close()
	return tao.NewLinuxHostAdminClient(conn).Shutdown()
}

func getKey(prompt, name string) []byte {
	if input := *options.String[name]; input != "" {
		// TODO(kwalsh) Maybe this should go to stderr?
		glog.Warning("Passwords on the command line are not secure. Use this only for testing")
		return []byte(input)
	} else {
		// Get the password from the user.
		fmt.Print(prompt + ": ")
		pwd, err := terminal.ReadPassword(syscall.Stdin)
		failIf(err, "Can't get password")
		fmt.Println()
		return pwd
	}
}

func failIf(err error, msg string, args ...interface{}) {
	if err != nil {
		fail(err, msg, args...)
	}
}

func fail(err error, msg string, args ...interface{}) {
	s := fmt.Sprintf(msg, args...)
	if err != nil {
		fmt.Fprintf(os.Stderr, "%v: %s\n", err, s)
	} else {
		fmt.Fprintf(os.Stderr, "error: %s\n", s)
	}
	os.Exit(2)
}

func usage(msg string, args ...interface{}) {
	s := fmt.Sprintf(msg, args...)
	fmt.Fprintf(os.Stderr, "%s\n", s)
	fmt.Fprintf(os.Stderr, "Try -help instead!\n")
	// help()
	os.Exit(1)
}
