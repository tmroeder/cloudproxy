// Copyright (c) 2015, Google Inc. All rights reserved.
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

package mixnet

import (
	"errors"
	"net"
)

const CellBytes = 1024 // Length of a cell

const (
	msgCell = 0x00
	dirCell = 0x01
)

var errCellLength error = errors.New("incorrect cell length")

// Conn implements the net.Conn interface. The read and write operations are
// overloaded to check that only cells are sent between agents in the mixnet
// protocol.
type Conn struct {
	net.Conn
}

// Read a cell from the channel. If len(msg) != CellBytes, return an error.
func (c Conn) Read(msg []byte) (n int, err error) {
	n, err = c.Conn.Read(msg)
	if err != nil {
		return n, err
	}
	if n != CellBytes {
		return n, errCellLength
	}
	return n, nil
}

// Write a cell to the channel. If the len(cell) != CellBytes, return an error.
func (c *Conn) Write(msg []byte) (n int, err error) {
	if len(msg) != CellBytes {
		return 0, errCellLength
	}
	n, err = c.Conn.Write(msg)
	if err != nil {
		return n, err
	}
	return n, nil
}

// padCell pads a message to the length of a cell and returns a byte slice of
// length CellBytes. If len(msg) > CellBytes, then return msg.
func padCell(msg []byte) []byte {
	if len(msg) >= CellBytes {
		return msg
	}

	cell := make([]byte, CellBytes) // Initially zero
	bytes := copy(cell, msg)
	cell[bytes] = 0xf0
	return cell
}

// unpadCell removes the padding from a message whose length is less than
// CellBytesa.
func unpadCell(cell []byte) []byte {
	i := len(cell) - 1
	for i > 0 && cell[i] == 0x00 {
		i--
	}
	if cell[i] == 0xf0 {
		msg := make([]byte, i)
		copy(msg, cell)
		return msg
	}
	return cell
}

// Write zeros to each byte of a cell.
func zeroCell(cell []byte) {
	for i := 0; i < CellBytes; i++ {
		cell[i] = 0
	}
}
