// Copyright 2009 Sebastien Martini (seb@dbzteam.org)
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
#ifndef KEYCZAR_OPENSSL_ECDSA_H_
#define KEYCZAR_OPENSSL_ECDSA_H_

#include <openssl/ecdsa.h>

#include <string>

#include <keyczar/base/basictypes.h>
#include <keyczar/base/scoped_ptr.h>
#include <keyczar/ecdsa_impl.h>
#include <keyczar/openssl/util.h>

namespace keyczar {

namespace openssl {

// OpenSSL concrete implementation.
class ECDSAOpenSSL : public ECDSAImpl {
 public:
  virtual ~ECDSAOpenSSL() {}

  // Builds an ECDSAOpenSSL object from |key|. |key| must be correctly formed
  // and initialized. The caller takes ownership over the returned object.
  static ECDSAOpenSSL* Create(const ECDSAIntermediateKey& key,
                              bool private_key);

  // Builds an ECDSAOpenSSL object from a new generated key of type |curve|.
  // The caller takes ownership over the returned instance.
  static ECDSAOpenSSL* GenerateKey(ECDSAImpl::Curve curve);

  // Builds an ECDSAOpenSSL object from a PEM private key stored at |filename|.
  // |passphrase| is an optional passphrase. Its value is NULL if no
  // passphrase is expected or if it should be prompted interactively at
  // execution. The caller takes ownership over the returned object.
  // It can handle PEM format keys as well as PKCS8 format keys.
  static ECDSAOpenSSL* CreateFromPEMPrivateKey(const std::string& filename,
                                               const std::string* passphrase);

  // Exports this key encrypted with |passphrase| to |filename|. The format
  // used is PKCS8 and the key is encrypted with PBE algorithm as defined in
  // PKCS5 v2.0, the associated cipher used is AES. If |passphrase| is NULL
  // a callback function will be called to prompt a passphrase at execution.
  virtual bool ExportPrivateKey(const std::string& filename,
                                const std::string* passphrase) const;

  virtual bool GetAttributes(ECDSAIntermediateKey* key);

  virtual bool GetPublicAttributes(ECDSAIntermediateKey* key);

  virtual bool Sign(const std::string& message_digest,
                    std::string* signature) const;

  virtual bool Verify(const std::string& message_digest,
                      const std::string& signature) const;

  virtual int Size() const;

  bool Equals(const ECDSAOpenSSL& rhs) const;

  bool private_key() const { return private_key_; }

  const EC_KEY* key() const { return key_.get(); }

 private:
  FRIEND_TEST(ECDSAOpenSSL, CreateKeyAndCompare);

  // EC_KEY_free internally calls BN_clear_free() to clear the
  // private field with OPENSSL_cleanse() before freeing the memory.
  typedef scoped_ptr_malloc<
      EC_KEY, openssl::OSSLDestroyer<EC_KEY, EC_KEY_free> > ScopedECKey;

  ECDSAOpenSSL(EC_KEY* key, bool private_key)
      : key_(key), private_key_(private_key) {}

  const ScopedECKey key_;

  bool private_key_;

  DISALLOW_COPY_AND_ASSIGN(ECDSAOpenSSL);
};

}  // namespace openssl

}  // namespace keyczar

#endif  // KEYCZAR_OPENSSL_ECDSA_H_
