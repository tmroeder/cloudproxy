#include <gflags/gflags.h>
#include <glog/logging.h>
#include <openssl/ssl.h>
#include "cloud_client.h"
#include "cloudproxy.pb.h"

#include <string>

using std::string;

DEFINE_string(client_cert, "./openssl_keys/client/client.crt",
		"The PEM certificate for the client to use for TLS");
DEFINE_string(client_key, "./openssl_keys/client/client.key",
		"The private key file for the client for TLS");

// this will be removed when get this password released by the TPM
DEFINE_string(client_password, "cpclient",
		"The private key file for the client for TLS");
DEFINE_string(policy_key, "./policy_public_key", "The keyczar public"
		" policy key");
DEFINE_string(pem_policy_key, "./openssl_keys/policy/policy.crt",
		"The PEM public policy cert");
DEFINE_string(address, "localhost", "The address of the local server");
DEFINE_int32(port, 11235, "The server port to connect to");

int main(int argc, char** argv) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    google::ParseCommandLineFlags(&argc, &argv, false);

    // initialize OpenSSL
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    SSL_library_init();

    LOG(INFO) << "About to create a client";
    cloudproxy::CloudClient cc(FLAGS_client_cert,
                               FLAGS_client_key,
			                   FLAGS_client_password,
                               FLAGS_policy_key,
                               FLAGS_pem_policy_key,
                               FLAGS_address, 
                               FLAGS_port);

    LOG(INFO) << "Created a client";
    CHECK(cc.Connect()) << "Could not connect to the server at "
                      << FLAGS_address << ":" << FLAGS_port;
    LOG(INFO) << "Connected to the server";
                  

    CHECK(cc.AddUser("tmroeder", "./keys/tmroeder", "tmroeder")) << "Could not"
      " add the user credential from its keyczar path";
    LOG(INFO) << "Added credentials for the user tmroeder";
    CHECK(cc.Authenticate("tmroeder", "./keys/tmroeder_pub_signed")) << "Could"
      " not authenticate tmroeder with the server";
    LOG(INFO) << "Authenticated to the server for tmroeder";
    CHECK(cc.Create("tmroeder", "test")) << "Could not create the object"
        " 'test' on the server";
    LOG(INFO) << "Created the object test";
    CHECK(cc.Read("tmroeder", "test")) << "Could not read the object";
    LOG(INFO) << "Read the object test";
    CHECK(cc.Destroy("tmroeder", "test")) << "Could not destroy the object";
    LOG(INFO) << "Destroyed the test object";

    CHECK(cc.Close(false)) << "Could not close the channel";
    
    LOG(INFO) << "Test succeeded";

    return 0;
}
