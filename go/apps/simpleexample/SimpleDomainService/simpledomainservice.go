// Copyright (c) 2014, Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package main

import (
	"bytes"
	"crypto/ecdsa"
	"crypto/tls"
	"crypto/rand"
	"crypto/x509"
	"crypto/x509/pkix"
	"errors"
	"flag"
	"fmt"
	"log"
	"math/big"
	"net"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/jlmucb/cloudproxy/go/tao"
	"github.com/jlmucb/cloudproxy/go/tao/auth"
	"github.com/jlmucb/cloudproxy/go/apps/simpleexample/domain_policy"
	"github.com/jlmucb/cloudproxy/go/util"
)

var network = flag.String("network", "tcp", "The network to use for connections")
var addr = flag.String("addr", "localhost:8124", "The address to listen on")
var domainPass = flag.String("password", "xxx", "The domain password")
var configPath = flag.String("config",
	"/Domains/domain.simpleexample/tao.config", "The Tao domain config")
var servicePath = flag.String("service path",
	"/Domains/domain.simpleexample/SimpleDomainService", "The Tao domain config")

var SerialNumber int64

func IsAuthenticationValid(name *string) bool {
fmt.Printf("IsAuthenticationValid\n")
	log.Printf("simpledomainservice, IsAuthenticationValid name is %s\n", *name)
	if name == nil {
		return false
	}
	log.Printf("simpledomainservice, IsAuthenticationValid returning true\n")
	return true
}

// First return is terminate flag.
func DomainRequest(conn net.Conn, policyKey *tao.Keys, guard tao.Guard) (bool, error) {
fmt.Printf("DomainRequest\n")
	log.Printf("DomainRequest\n")

	// Expect a request with attestation from client.
	ms := util.NewMessageStream(conn)
	var request domain_policy.DomainCertRequest
	err := ms.ReadMessage(&request)
	if err != nil {
		log.Printf("DomainRequest: Couldn't read attestation from channel:", err)
		log.Printf("\n")
		return false, err
	}

	var a tao.Attestation
	err = proto.Unmarshal(request.Attestation, &a)
	if request.KeyType == nil  {
		log.Printf("Domain: Empty key type")
		return false, errors.New("Empty key type")
	}
	if *request.KeyType != "ECDSA"  {
		log.Printf("Domain: bad key type")
		return false, errors.New("Domain: bad key type")
	}
	subjectPublicKey, err := domain_policy.GetEcdsaKeyFromDer(request.SubjectPublicKey)
	if err != nil {
		log.Printf("DomainRequest: can't get key from der")
		return false, errors.New("DomainRequest: can't get key from der")
	}

	// Get hash of the public key subject.
	serializedKey, err := domain_policy.SerializeEcdsaKeyToInternalName(subjectPublicKey.(*ecdsa.PublicKey))
	if err!= nil || serializedKey == nil {
		log.Printf("DomainRequest: Can't serialize key to internal format\n")
		return false, errors.New("DomainRequest: Can't serialize key to internal format")
	}
	subjectKeyHash := domain_policy.GetKeyHash(serializedKey)

	peerCert := conn.(*tls.Conn).ConnectionState().PeerCertificates[0]
	err = tao.ValidatePeerAttestation(&a, peerCert, guard)
/*
	if err != nil {
		log.Printf("Domain: RequestCouldn't validate peer attestation:", err)
		return false, err
	}
fmt.Printf("DomainRequest, peerCert: %x\n", peerCert)
*/

	// Sign cert

	// Get Program name and key info from delegation.
	f, err := auth.UnmarshalForm(a.SerializedStatement)
	if err != nil {
		log.Printf("DomainRequest: Can't unmarshal a.SerializedStatement\n")
		return false, err
	}

	var saysStatement *auth.Says
	if ptr, ok := f.(*auth.Says); ok {
		saysStatement = ptr
	} else if val, ok := f.(auth.Says); ok {
		saysStatement = &val
	}
	sf, ok := saysStatement.Message.(auth.Speaksfor)
	if ok != true {
		log.Printf("DomainRequest: says doesnt have speaksfor message\n")
		return false, err
	}
	// this in the new key principal
	clientKeyPrincipal, ok := sf.Delegate.(auth.Prin)
	if ok != true {
		log.Printf("DomainRequest: speaksfor Delegate is not auth.Prin\n")
		return false, err
	}

	programPrincipal, ok := sf.Delegator.(auth.Prin)
	if ok != true {
		log.Printf("DomainRequest: Can't get subject principal\n")
		return false, errors.New("Can't get principal name from verifier")
	}
	programPrincipalName:= programPrincipal.String()
	verified := IsAuthenticationValid(&programPrincipalName)
	if !verified {
		log.Printf("DomainRequest: name verification failed\n")
		return false, err
	}
fmt.Printf("\nSimpleDomainService: key principal: %s, program principal: %s\n", clientKeyPrincipal, programPrincipalName)

	// Is the delegate the same key as was presented in the name in the request?
	namedHash := clientKeyPrincipal.KeyHash.(auth.Bytes)
fmt.Printf("\nkeyhash: %x\n", namedHash)
	if bytes.Compare(subjectKeyHash[:], namedHash) != 0 {
		log.Printf("DomainRequest: named hash is wrong\n")
fmt.Printf("DomainRequest: named hash is wrong, named: %x, computed: %x\n",
			namedHash, subjectKeyHash)
		return false, errors.New("DomainRequest: named hash is wrong") 
	}

	// Sign program certificate.

	notBefore := time.Now()
	validFor := 365*24*time.Hour
        notAfter := notBefore.Add(validFor)

	us := "US"
	issuerName := "Google"
	localhost := "localhost"
	x509SubjectName :=  &pkix.Name {
		Organization:       []string{programPrincipalName},
		OrganizationalUnit: []string{programPrincipalName},
		CommonName:	    localhost,
		Country:	    []string{us},
	}
	x509IssuerName :=  &pkix.Name {
		Organization:       []string {issuerName},
		OrganizationalUnit: []string {issuerName},
		CommonName:	    localhost,
		Country:	    []string{us},
	}

	// issuerName := tao.NewX509Name(&details)
	SerialNumber = SerialNumber + 1
	var sn big.Int
	certificateTemplate := x509.Certificate {
		SerialNumber:		&sn,
		Issuer:			*x509IssuerName,
		Subject:		*x509SubjectName,
		NotBefore:		notBefore,
		NotAfter:		notAfter,
		KeyUsage:		x509.KeyUsageCertSign |
		x509.KeyUsageKeyAgreement | x509.KeyUsageDigitalSignature,
	}

	clientCert, err := x509.CreateCertificate(rand.Reader, &certificateTemplate,
                            policyKey.Cert, subjectPublicKey,
                            policyKey.SigningKey.GetSigner())
	if err != nil {
		fmt.Printf("Can't create client certificate: ", err, "\n")
		return false, err
	}

	zero := int32(0)
	var ra domain_policy.DomainCertResponse;
        ra.Error = &zero
        ra.SignedCert= clientCert

	// Add cert chain (just policy cert for now).
	ra.CertChain= append(ra.CertChain, policyKey.Cert.Raw)

	_, err = ms.WriteMessage(&ra)
	if err != nil {
		log.Printf("DomainRequest: Couldn't return the attestation on the channel: ", err)
		log.Printf("\n")
		return false, err
	}
	return false, nil
}

func main() {
	flag.Parse()
	domain, err := tao.LoadDomain(*configPath, []byte(*domainPass))
	if domain == nil {
		log.Printf("simpledomainservice: no domain path - %s, pass - %s, err - %s\n",
			*configPath, *domainPass, err)
		return
	} else if err != nil {
		log.Printf("simpledomainservice: Couldn't load the config path %s: %s\n",
			*configPath, err)
		return
	}
	log.Printf("simpledomainservice: Loaded domain\n")

	// Set up temporary keys for the connection, since the only thing that
	// matters to the remote client is that they receive a correctly-signed new
	// attestation from the policy key.
	// JLM:  I left this in place but I'm not sure what a TLS connection with a
	//   self signed Cert buys in terms of security.
	//   The security of this protocol should not depend on the
	//   confidentiality or intergity of the channel.  All that said, if we
	//   do ever distribute a signed simpledomainservice cert
	// for this TLS channel, it would be good.
	keys, err := tao.NewTemporaryKeys(tao.Signing)
	if keys == nil || err != nil {
		log.Fatalln("simpledomainservice: Couldn't set up temporary keys for connection:", err)
		return
	}
	keys.Cert, err = keys.SigningKey.CreateSelfSignedX509(&pkix.Name{
		Organization: []string{"Google Tao Demo"}})
	if err != nil {
		log.Fatalln("simpledomainservice: Couldn't set up a self-signed cert:", err)
		return
	}
	SerialNumber = int64(time.Now().UnixNano()) / (1000000)
	policyKey := domain.Keys
fmt.Printf("\nSimpleDomainService: policyKey: %x\n", policyKey)

	tlsc, err := tao.EncodeTLSCert(keys)
	if err != nil {
		log.Fatalln("simpledomainservice: Couldn't encode a TLS cert:", err)
	}
	conf := &tls.Config{
		RootCAs:            x509.NewCertPool(),
		Certificates:       []tls.Certificate{*tlsc},
		InsecureSkipVerify: true,
		ClientAuth:         tls.RequireAnyClientCert,
	}
	sock, err := tls.Listen(*network, *addr, conf)
	if err != nil {
		log.Printf("simpledomainservice: error: %s\n", err)
	}
	if sock == nil {
		log.Printf("simpledomainservice: Empty socket, terminating\n")
		return
	}
	defer sock.Close()

	log.Printf("simpledomainservice: accepting connections\n")
fmt.Printf("\n\nsimpledomainservice: accepting connections\n")
	for {
		conn, err := sock.Accept()
		if conn == nil {
fmt.Printf("simpledomainservice: Empty connection\n")
			log.Printf("simpledomainservice: Empty connection\n")
			return
		} else if err != nil {
fmt.Printf("simpledomainservice: Couldn't accept a connection on %s: %s\n", *addr, err)
			log.Printf("simpledomainservice: Couldn't accept a connection on %s: %s\n", *addr, err)
			return
		}
		go DomainRequest(conn, policyKey, domain.Guard)
	}
	log.Printf("simpledomainservice: finishing\n")
}
