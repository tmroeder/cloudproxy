// Copyright (c) 2014, Google Inc.  All rights reserved.
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

package tao

import (
	"github.com/jlmucb/cloudproxy/go/tao/auth"
)

// Host is a generic interface for a Tao host that can be configured and driven
// by a variety of host environments. Generally, the host environment is
// responsible for enforcing and managing policy, managing hosted programs (e.g.
// measuring, naming, starting, stopping), communication with hosted programs
// (e.g. channel creation, RPC reception), and other host-specific details.
//
// Because the environment calls Host in response to requests from hosted
// processes invoking the Tao interface, several Host methods resemble methods
// in Tao. Semantics and method signatures differ slightly, however, since the
// environment can add context (e.g., the subprincipal name of the requesting
// child) or do part of the implementation (e.g., manage policy on seal/unseal).
type Host interface {
	// GetRandomBytes returns a slice of n random bytes.
	GetRandomBytes(childSubprin auth.SubPrin, n int) (bytes []byte, err error)

	// GetSharedSecret returns a slice of n secret bytes.
	GetSharedSecret(tag string, n int) (bytes []byte, err error)

	// Attest requests the Tao host sign a statement on behalf of the caller.
	Attest(childSubprin auth.SubPrin, issuer *auth.Prin,
		time, expiration *int64, message auth.Form) (*Attestation, error)

	// Encrypt data so that only this host can access it.
	Encrypt(data []byte) (encrypted []byte, err error)

	// Decrypt data that only this host can access.
	Decrypt(encrypted []byte) (data []byte, err error)

	// Notify this Host that a new hosted program has been created.
	AddedHostedProgram(childSubprin auth.SubPrin) error

	// Notify this Host that a hosted program has been killed.
	RemovedHostedProgram(childSubprin auth.SubPrin) error

	// Get the Tao principal name assigned to this hosted Tao host. The
	// name encodes the full path from the root Tao, through all
	// intermediary Tao hosts, to this hosted Tao host.
	HostName() auth.Prin
}
