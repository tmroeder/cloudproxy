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
	"crypto/x509/pkix"
	"flag"
	"fmt"
	"net"
	"os"
	"strings"

	"github.com/golang/glog"
	"github.com/jlmucb/cloudproxy/go/tao"
)

var serverHost = flag.String("host", "localhost", "address for client/server")
var serverPort = flag.String("port", "8123", "port for client/server")
var serverAddr string // see main()
var pingCount = flag.Int("n", 5, "Number of client/server pings")
var demoAuth = flag.String("auth", "tao", "\"tcp\", \"tls\", or \"tao\"")
var configPath = flag.String("config", "tao.config", "The Tao domain config")
var ca = flag.String("ca", "", "address for Tao CA, if any")

var subprinRule = "(forall P: forall Hash: TrustedProgramHash(Hash) and Subprin(P, %v, Hash) implies Authorized(P, \"Execute\"))"

func doRequest(guard tao.Guard, domain *tao.Domain, keys *tao.Keys) bool {
	glog.Infof("client: connecting to %s using %s authentication.\n", serverAddr, *demoAuth)
	var conn net.Conn
	var err error
	network := "tcp"

	switch *demoAuth {
	case "tcp":
		conn, err = net.Dial(network, serverAddr)
	case "tls":
		conn, err = tao.DialTLSWithKeys(network, serverAddr, keys)
	case "tao":
		conn, err = tao.Dial(network, serverAddr, guard, domain.Keys.VerifyingKey, keys)
	}
	if err != nil {
		glog.Infof("client: error connecting to %s: %s\n", serverAddr, err.Error())
		return false
	}
	defer conn.Close()

	_, err = fmt.Fprintf(conn, "Hello\n")
	if err != nil {
		glog.Infof("client: can't write: %s\n", err.Error())
		return false
	}
	msg, err := bufio.NewReader(conn).ReadString('\n')
	if err != nil {
		glog.Infof("client: can't read: %s\n", err.Error())
		return false
	}
	msg = strings.TrimSpace(msg)
	glog.Infof("client: got reply: %s\n", msg)
	return true
}

func newTempCAGuard(v *tao.Verifier) (tao.Guard, error) {
	g := tao.NewTemporaryDatalogGuard()
	vprin := v.ToPrincipal()
	rule := fmt.Sprintf(subprinRule, vprin)

	if err := g.AddRule(rule); err != nil {
		return nil, err
	}

	return g, nil
}

func doClient(domain *tao.Domain) {
	network := "tcp"
	keys, err := tao.NewTemporaryTaoDelegatedKeys(tao.Signing, tao.Parent())
	if err != nil {
		glog.Fatalf("client: couldn't generate temporary Tao keys: %s\n", err)
	}

	// TODO(tmroeder): fix the name
	cert, err := keys.SigningKey.CreateSelfSignedX509(&pkix.Name{
		Organization: []string{"Google Tao Demo"}})
	if err != nil {
		glog.Fatalf("client: couldn't create a self-signed X.509 cert: %s\n", err)
	}
	// TODO(kwalsh) keys should save cert on disk if keys are on disk
	keys.Cert = cert

	g := domain.Guard
	if *ca != "" {
		na, err := tao.RequestTruncatedAttestation(network, *ca, keys, domain.Keys.VerifyingKey)
		if err != nil {
			glog.Fatalf("client: couldn't get a truncated attestation from %s: %s\n", *ca, err)
		}

		keys.Delegation = na

		// If we're using a CA, then use a custom guard that accepts only
		// programs that have talked to the CA.
		g, err = newTempCAGuard(domain.Keys.VerifyingKey)
		if err != nil {
			glog.Fatalf("client: couldn't set up a new guard: %s\n", err)
		}
	}

	pingGood := 0
	pingFail := 0
	for i := 0; i < *pingCount || *pingCount < 0; i++ { // negative means forever
		if doRequest(g, domain, keys) {
			pingGood++
		} else {
			pingFail++
		}
		glog.Infof("client: made %d connections, finished %d ok, %d bad pings\n", i+1, pingGood, pingFail)
		fmt.Printf("client: made %d connections, finished %d ok, %d bad pings\n", i+1, pingGood, pingFail)
	}
}

func main() {
	flag.Parse()
	glog.Info("In demo_client")

	// Check to see if we are running in Docker mode with linked containers.
	// If so, then there will be an environment variable SERVER_PORT that
	// will contain a value of the form tcp://<ip>:<port>
	serverEnvVar := os.Getenv("SERVER_PORT")
	if serverEnvVar == "" {
		serverAddr = net.JoinHostPort(*serverHost, *serverPort)
	} else {
		serverAddr = strings.TrimPrefix(serverEnvVar, "tcp://")
		if serverAddr == serverEnvVar {
			glog.Fatalf("client: invalid SERVER_PORT environment variable value '%s'\n", serverEnvVar)
		}
	}

	switch *demoAuth {
	case "tcp", "tls", "tao":
	default:
		glog.Fatalf("unrecognized authentication mode: %s\n", *demoAuth)
	}

	glog.Info("Go Tao Demo")
	fmt.Println("Go Tao Demo")

	if tao.Parent() == nil {
		glog.Fatal("can't continue: No host Tao available")
	}

	domain, err := tao.LoadDomain(*configPath, nil)
	if err != nil {
		glog.Fatalf("error: couldn't load the tao domain from %s\n", *configPath)
	}

	doClient(domain)
	glog.Info("Client Done")
	glog.Flush()
	fmt.Println("Client Done")
}
