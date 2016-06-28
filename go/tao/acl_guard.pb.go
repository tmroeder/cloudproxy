// Code generated by protoc-gen-go.
// source: acl_guard.proto
// DO NOT EDIT!

/*
Package tao is a generated protocol buffer package.

It is generated from these files:
	acl_guard.proto
	attestation.proto
	ca.proto
	datalog_guard.proto
	domain.proto
	keys.proto
	linux_host_admin_rpc.proto
	linux_host.proto
	rpc.proto
	tpm_tao.proto

It has these top-level messages:
	ACLSet
	SignedACLSet
	Attestation
	CARequest
	CAResponse
	DatalogRules
	SignedDatalogRules
	DomainDetails
	X509Details
	ACLGuardDetails
	DatalogGuardDetails
	TPMDetails
	TPM2Details
	DomainConfig
	DomainTemplate
	CryptoKey
	CryptoKeyset
	PBEData
	ECDSA_SHA_VerifyingKeyV1
	ECDSA_SHA_SigningKeyV1
	AES_CTR_HMAC_SHA_CryptingKeyV1
	HMAC_SHA_DerivingKeyV1
	CryptoHeader
	SignaturePDU
	SignedData
	EncryptedData
	KeyDerivationPDU
	LinuxHostAdminRPCRequest
	LinuxHostAdminRPCHostedProgram
	LinuxHostAdminRPCResponse
	LinuxHostSealedBundle
	LinuxHostConfig
	RPCRequest
	RPCResponse
	HybridSealedData
*/
package tao

import proto "github.com/golang/protobuf/proto"
import fmt "fmt"
import math "math"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// This is a compile-time assertion to ensure that this generated file
// is compatible with the proto package it is being compiled against.
const _ = proto.ProtoPackageIsVersion1

// A set of ACL entries.
type ACLSet struct {
	Entries          []string `protobuf:"bytes,1,rep,name=entries" json:"entries,omitempty"`
	XXX_unrecognized []byte   `json:"-"`
}

func (m *ACLSet) Reset()                    { *m = ACLSet{} }
func (m *ACLSet) String() string            { return proto.CompactTextString(m) }
func (*ACLSet) ProtoMessage()               {}
func (*ACLSet) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{0} }

func (m *ACLSet) GetEntries() []string {
	if m != nil {
		return m.Entries
	}
	return nil
}

// A set of ACL entries signed by a key.
type SignedACLSet struct {
	SerializedAclset []byte `protobuf:"bytes,1,req,name=serialized_aclset" json:"serialized_aclset,omitempty"`
	Signature        []byte `protobuf:"bytes,2,req,name=signature" json:"signature,omitempty"`
	XXX_unrecognized []byte `json:"-"`
}

func (m *SignedACLSet) Reset()                    { *m = SignedACLSet{} }
func (m *SignedACLSet) String() string            { return proto.CompactTextString(m) }
func (*SignedACLSet) ProtoMessage()               {}
func (*SignedACLSet) Descriptor() ([]byte, []int) { return fileDescriptor0, []int{1} }

func (m *SignedACLSet) GetSerializedAclset() []byte {
	if m != nil {
		return m.SerializedAclset
	}
	return nil
}

func (m *SignedACLSet) GetSignature() []byte {
	if m != nil {
		return m.Signature
	}
	return nil
}

func init() {
	proto.RegisterType((*ACLSet)(nil), "tao.ACLSet")
	proto.RegisterType((*SignedACLSet)(nil), "tao.SignedACLSet")
}

var fileDescriptor0 = []byte{
	// 125 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x09, 0x6e, 0x88, 0x02, 0xff, 0xe2, 0xe2, 0x4f, 0x4c, 0xce, 0x89,
	0x4f, 0x2f, 0x4d, 0x2c, 0x4a, 0xd1, 0x2b, 0x28, 0xca, 0x2f, 0xc9, 0x17, 0x62, 0x2e, 0x49, 0xcc,
	0x57, 0x92, 0xe4, 0x62, 0x73, 0x74, 0xf6, 0x09, 0x4e, 0x2d, 0x11, 0xe2, 0xe7, 0x62, 0x4f, 0xcd,
	0x2b, 0x29, 0xca, 0x4c, 0x2d, 0x96, 0x60, 0x54, 0x60, 0xd6, 0xe0, 0x54, 0xb2, 0xe1, 0xe2, 0x09,
	0xce, 0x4c, 0xcf, 0x4b, 0x4d, 0x81, 0x2a, 0x90, 0xe4, 0x12, 0x2c, 0x4e, 0x2d, 0xca, 0x4c, 0xcc,
	0xc9, 0xac, 0x4a, 0x4d, 0x89, 0x07, 0x9a, 0x56, 0x9c, 0x5a, 0x02, 0x54, 0xca, 0xa4, 0xc1, 0x23,
	0x24, 0xc8, 0xc5, 0x59, 0x0c, 0x54, 0x9a, 0x58, 0x52, 0x5a, 0x94, 0x2a, 0xc1, 0x04, 0x12, 0x02,
	0x04, 0x00, 0x00, 0xff, 0xff, 0xe6, 0x6e, 0x44, 0xe5, 0x6f, 0x00, 0x00, 0x00,
}
