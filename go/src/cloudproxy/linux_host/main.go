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
	"bufio"
	"crypto/tls"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"errors"
	"flag"
	"fmt"
	"net"
	"os"
	"strconv"
	"strings"
	"time"

	"cloudproxy/tao"
	"cloudproxy/util"
)

var configPath = flag.String("config_path", "tao.config", "Location of tao domain configuration")
var path = flag.String("path", "linux_tao_host", "Location of linux host configuration")
var root = flag.Bool("root", false, "Run in root mode")
var stacked = flag.Bool("stacked", false, "Run in stacked mode")
var pass = flag.String("pass", "", "Password for unlocking keys if running in root mode")

var create = flag.Bool("create", false, "Create a new LinuxHost service.")
var show = flag.Bool("show", false, "Show principal name for LinuxHost service.")
var service = flag.Bool("service", false, "Start the LinuxHost service.")
var shutdown = flag.Bool("shutdown", false, "Shut down the LinuxHost service.")

var run = flag.Bool("run", false, "Start a hosted program (path and args follow --).")
var list = flag.Bool("list", false, "List hosted programs.")
var stop = flag.Bool("stop", false, "Stop a hosted program (names follow --).")
var kill = flag.Bool("kill", false, "Kill a hosted program (names follow --).")
var name = flag.Bool("name", false, "Show the principal name of running LinuxHost.")

func countBools(b ...bool) {
	var n int
	for b := range b {
		if b {
			n++
		}
	}
	return n
}

func main() {
	if flag.Parse() == flag.ErrHelp {
		fmt.Printf(`Administrative utility for LinuxHost.
Usage:
%[1]s [options] -create
%[1]s [options] -show
%[1]s [options] -service
%[1]s [options] -shutdown
%[1]s [options] -run -- program args...
%[1]s [options] -stop -- progname...
%[1]s [options] -kill -- progname...
%[1]s [options] -list
%[1]s [options] -name`, os.Args[0])
	}

	if countBools(create, show, service, shutdown, run, stop, kill, list, name) > 1 {
		fmt.Printf("error: specify at most one of the command options\n")
		return
	}

	if create || service || show {
		// load config
		// create host
		if root {
			// init
		} else if stacked {
			// init
		} else {
			fmt.Printf("error: must specify either -root or -stacked")
		}
		if create {
			fmt.Printf("LinuxHost Service: %s\n", host.TaoHostName())
		} else if show {
      fmt.Printf("export GOOGLE_TAO_LINUX='%v'\n", host.TaoHostName())
		} else /* service */ {
      fmt.Printf("Linux Tao Service started and waiting for requests\n")
			// listen
		}
	} else {
		// connect
		if shutdown {
			// shutdown
		} else if run {
			// run
		} else if kill || stop {
			// kill or stop
		} else if list {
			// list
		} else if name {
			fmt.Printf("%v\n", name)
		} else {
			printf("LinuxHost: %s\n", name)
		}
	}
}