// Code generated by protoc-gen-go.
// source: linux_admin_rpc.proto
// DO NOT EDIT!

package tao

import proto "code.google.com/p/goprotobuf/proto"
import math "math"

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = math.Inf

type LinuxAdminRPCRequestType int32

const (
	LinuxAdminRPCRequestType_LINUX_ADMIN_RPC_UNKNOWN              LinuxAdminRPCRequestType = 0
	LinuxAdminRPCRequestType_LINUX_ADMIN_RPC_SHUTDOWN             LinuxAdminRPCRequestType = 1
	LinuxAdminRPCRequestType_LINUX_ADMIN_RPC_START_HOSTED_PROGRAM LinuxAdminRPCRequestType = 2
	LinuxAdminRPCRequestType_LINUX_ADMIN_RPC_STOP_HOSTED_PROGRAM  LinuxAdminRPCRequestType = 3
	LinuxAdminRPCRequestType_LINUX_ADMIN_RPC_KILL_HOSTED_PROGRAM  LinuxAdminRPCRequestType = 4
	LinuxAdminRPCRequestType_LINUX_ADMIN_RPC_GET_TAO_HOST_NAME    LinuxAdminRPCRequestType = 5
	LinuxAdminRPCRequestType_LINUX_ADMIN_RPC_LIST_HOSTED_PROGRAMS LinuxAdminRPCRequestType = 6
)

var LinuxAdminRPCRequestType_name = map[int32]string{
	0: "LINUX_ADMIN_RPC_UNKNOWN",
	1: "LINUX_ADMIN_RPC_SHUTDOWN",
	2: "LINUX_ADMIN_RPC_START_HOSTED_PROGRAM",
	3: "LINUX_ADMIN_RPC_STOP_HOSTED_PROGRAM",
	4: "LINUX_ADMIN_RPC_KILL_HOSTED_PROGRAM",
	5: "LINUX_ADMIN_RPC_GET_TAO_HOST_NAME",
	6: "LINUX_ADMIN_RPC_LIST_HOSTED_PROGRAMS",
}
var LinuxAdminRPCRequestType_value = map[string]int32{
	"LINUX_ADMIN_RPC_UNKNOWN":              0,
	"LINUX_ADMIN_RPC_SHUTDOWN":             1,
	"LINUX_ADMIN_RPC_START_HOSTED_PROGRAM": 2,
	"LINUX_ADMIN_RPC_STOP_HOSTED_PROGRAM":  3,
	"LINUX_ADMIN_RPC_KILL_HOSTED_PROGRAM":  4,
	"LINUX_ADMIN_RPC_GET_TAO_HOST_NAME":    5,
	"LINUX_ADMIN_RPC_LIST_HOSTED_PROGRAMS": 6,
}

func (x LinuxAdminRPCRequestType) Enum() *LinuxAdminRPCRequestType {
	p := new(LinuxAdminRPCRequestType)
	*p = x
	return p
}
func (x LinuxAdminRPCRequestType) String() string {
	return proto.EnumName(LinuxAdminRPCRequestType_name, int32(x))
}
func (x *LinuxAdminRPCRequestType) UnmarshalJSON(data []byte) error {
	value, err := proto.UnmarshalJSONEnum(LinuxAdminRPCRequestType_value, data, "LinuxAdminRPCRequestType")
	if err != nil {
		return err
	}
	*x = LinuxAdminRPCRequestType(value)
	return nil
}

type LinuxAdminRPCRequest struct {
	Rpc              *LinuxAdminRPCRequestType `protobuf:"varint,1,req,name=rpc,enum=tao.LinuxAdminRPCRequestType" json:"rpc,omitempty"`
	Seq              *uint64                   `protobuf:"varint,2,req,name=seq" json:"seq,omitempty"`
	Data             []byte                    `protobuf:"bytes,3,opt,name=data" json:"data,omitempty"`
	Path             *string                   `protobuf:"bytes,4,opt,name=path" json:"path,omitempty"`
	Args             []string                  `protobuf:"bytes,5,rep,name=args" json:"args,omitempty"`
	XXX_unrecognized []byte                    `json:"-"`
}

func (m *LinuxAdminRPCRequest) Reset()         { *m = LinuxAdminRPCRequest{} }
func (m *LinuxAdminRPCRequest) String() string { return proto.CompactTextString(m) }
func (*LinuxAdminRPCRequest) ProtoMessage()    {}

func (m *LinuxAdminRPCRequest) GetRpc() LinuxAdminRPCRequestType {
	if m != nil && m.Rpc != nil {
		return *m.Rpc
	}
	return LinuxAdminRPCRequestType_LINUX_ADMIN_RPC_UNKNOWN
}

func (m *LinuxAdminRPCRequest) GetSeq() uint64 {
	if m != nil && m.Seq != nil {
		return *m.Seq
	}
	return 0
}

func (m *LinuxAdminRPCRequest) GetData() []byte {
	if m != nil {
		return m.Data
	}
	return nil
}

func (m *LinuxAdminRPCRequest) GetPath() string {
	if m != nil && m.Path != nil {
		return *m.Path
	}
	return ""
}

func (m *LinuxAdminRPCRequest) GetArgs() []string {
	if m != nil {
		return m.Args
	}
	return nil
}

type LinuxAdminRPCHostedProgramList struct {
	Name             []string `protobuf:"bytes,1,rep,name=name" json:"name,omitempty"`
	Pid              []int32  `protobuf:"varint,2,rep,name=pid" json:"pid,omitempty"`
	XXX_unrecognized []byte   `json:"-"`
}

func (m *LinuxAdminRPCHostedProgramList) Reset()         { *m = LinuxAdminRPCHostedProgramList{} }
func (m *LinuxAdminRPCHostedProgramList) String() string { return proto.CompactTextString(m) }
func (*LinuxAdminRPCHostedProgramList) ProtoMessage()    {}

func (m *LinuxAdminRPCHostedProgramList) GetName() []string {
	if m != nil {
		return m.Name
	}
	return nil
}

func (m *LinuxAdminRPCHostedProgramList) GetPid() []int32 {
	if m != nil {
		return m.Pid
	}
	return nil
}

type LinuxAdminRPCResponse struct {
	Rpc   *LinuxAdminRPCRequestType `protobuf:"varint,1,req,name=rpc,enum=tao.LinuxAdminRPCRequestType" json:"rpc,omitempty"`
	Seq   *uint64                   `protobuf:"varint,2,req,name=seq" json:"seq,omitempty"`
	Error *string                   `protobuf:"bytes,3,opt,name=error" json:"error,omitempty"`
	Data  []byte                    `protobuf:"bytes,4,opt,name=data" json:"data,omitempty"`
	// serialized LinuxAdminRPCHostedProgramList
	Reason           *string `protobuf:"bytes,5,opt,name=reason" json:"reason,omitempty"`
	XXX_unrecognized []byte  `json:"-"`
}

func (m *LinuxAdminRPCResponse) Reset()         { *m = LinuxAdminRPCResponse{} }
func (m *LinuxAdminRPCResponse) String() string { return proto.CompactTextString(m) }
func (*LinuxAdminRPCResponse) ProtoMessage()    {}

func (m *LinuxAdminRPCResponse) GetRpc() LinuxAdminRPCRequestType {
	if m != nil && m.Rpc != nil {
		return *m.Rpc
	}
	return LinuxAdminRPCRequestType_LINUX_ADMIN_RPC_UNKNOWN
}

func (m *LinuxAdminRPCResponse) GetSeq() uint64 {
	if m != nil && m.Seq != nil {
		return *m.Seq
	}
	return 0
}

func (m *LinuxAdminRPCResponse) GetError() string {
	if m != nil && m.Error != nil {
		return *m.Error
	}
	return ""
}

func (m *LinuxAdminRPCResponse) GetData() []byte {
	if m != nil {
		return m.Data
	}
	return nil
}

func (m *LinuxAdminRPCResponse) GetReason() string {
	if m != nil && m.Reason != nil {
		return *m.Reason
	}
	return ""
}

func init() {
	proto.RegisterEnum("tao.LinuxAdminRPCRequestType", LinuxAdminRPCRequestType_name, LinuxAdminRPCRequestType_value)
}
