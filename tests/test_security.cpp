#include <cassert>
#include <cstring>
#include <vector>
#ifdef __APPLE__
#include <CommonCrypto/CommonHMAC.h>
#include <CommonCrypto/CommonCryptor.h>
#else
#include <openssl/evp.h>
#include <openssl/hmac.h>
#endif
#include "../src/shared/Ath10kSupplicant.hpp"
#include "../src/shared/Ath10kControl.hpp"
#include "security_vectors.inc"
#include "rekey_vectors.inc"
#ifdef __APPLE__
struct OpenSSLCrypto {
 static bool random(uint8_t *p,size_t n){for(size_t i=0;i<n;++i)p[i]=uint8_t(i+51);return true;}
 static bool hmac(const uint8_t *k,size_t kl,const uint8_t *p,size_t n,uint8_t *o){CCHmac(kCCHmacAlgSHA1,k,kl,p,n,o);return true;}
 static bool decrypt(const uint8_t *k,const uint8_t *p,uint8_t *o){size_t done=0;return CCCrypt(kCCDecrypt,kCCAlgorithmAES,kCCOptionECBMode,k,16,nullptr,p,16,o,16,&done)==kCCSuccess && done==16;}
};
#else
struct OpenSSLCrypto {
 static bool random(uint8_t *p,size_t n){for(size_t i=0;i<n;++i)p[i]=uint8_t(i+51);return true;}
 static bool hmac(const uint8_t *k,size_t kl,const uint8_t *p,size_t n,uint8_t *o){unsigned len=0;return HMAC(EVP_sha1(),k,int(kl),p,n,o,&len)&&len==20;}
 static bool decrypt(const uint8_t *k,const uint8_t *p,uint8_t *o){
  EVP_CIPHER_CTX *c=EVP_CIPHER_CTX_new();assert(c);int n=0,last=0;
  bool ok=EVP_DecryptInit_ex(c,EVP_aes_128_ecb(),nullptr,k,nullptr)==1&&EVP_CIPHER_CTX_set_padding(c,0)==1&&EVP_DecryptUpdate(c,o,&n,p,16)==1&&EVP_DecryptFinal_ex(c,o+n,&last)==1&&n+last==16;
  EVP_CIPHER_CTX_free(c);return ok;
 }
};
#endif
using S=AthSecurity::Supplicant<OpenSSLCrypto>;
static void start(S &s){assert(s.begin(v_pmk,v_snonce,v_sta,v_ap,v_rsn,sizeof(v_rsn)));}
static void rekeyTests(){
 S s;start(s);assert(s.receive(v_m1,sizeof(v_m1))==S::Reply);assert(s.receive(v_m3,sizeof(v_m3))==S::Keys);
 assert(s.receive(r_group_new,sizeof(r_group_new))==S::GroupKeys);
 assert(!memcmp(s.keys+16,r_new_gtk,16) && s.keys[32]==2 && s.keys[33]==9);
 assert(s.outputLength==sizeof(r_group_new_ack) && !memcmp(s.output,r_group_new_ack,sizeof(r_group_new_ack)));
 for(unsigned i=0;i<10;++i)assert(s.receive(r_group_new,sizeof(r_group_new))==S::Reply);
 assert(s.receive(v_group,sizeof(v_group))==S::Ignore);
 assert(s.receive(r_group_same,sizeof(r_group_same))==S::Reply);
 // Same replay with changed payload/MIC cannot cause a reinstall or an ACK.
 std::vector<uint8_t> changed(r_group_same,r_group_same+sizeof(r_group_same));changed.back()^=1;
 assert(s.receive(changed.data(),changed.size())==S::Ignore);
 assert(s.receive(r_group_other_index,sizeof(r_group_other_index))==S::Fatal);
 start(s);assert(s.receive(v_m1,sizeof(v_m1))==S::Reply);assert(s.receive(v_m3,sizeof(v_m3))==S::Keys);
 assert(s.receive(r_pair1,sizeof(r_pair1))==S::Reply && s.pairwisePending());
 assert(s.outputLength==sizeof(r_pair2) && !memcmp(s.output,r_pair2,sizeof(r_pair2)));
 assert(s.receive(r_pair1,sizeof(r_pair1))==S::Reply);
 changed.assign(r_pair3,r_pair3+sizeof(r_pair3));changed[95]^=1;
 assert(s.receive(changed.data(),changed.size())==S::Ignore && s.pairwisePending());
 s.cancelPairwiseAttempt();assert(!s.pairwisePending());
 // Forged M1 timeout restores established keys: an authentic GTK rekey still works.
 assert(s.receive(r_group_new,sizeof(r_group_new))==S::GroupKeys);
 assert(s.receive(r_pair1,sizeof(r_pair1))==S::Reply);
 assert(s.receive(r_pair3,sizeof(r_pair3))==S::PairKeys && !s.pairwisePending());
 assert(!memcmp(s.keys,r_new_ptk+32,16) && !memcmp(s.keys+16,r_new_gtk,16));
 assert(s.outputLength==sizeof(r_pair4) && !memcmp(s.output,r_pair4,sizeof(r_pair4)));
 for(unsigned i=0;i<10;++i)assert(s.receive(r_pair3,sizeof(r_pair3))==S::Reply);
 assert(s.receive(v_m3,sizeof(v_m3))==S::Ignore);
 assert(s.receive(r_group_after_pair,sizeof(r_group_after_pair))==S::GroupKeys);
 s.reset();assert(s.receive(r_pair1,sizeof(r_pair1))==S::Ignore);
}

int main(){
 rekeyTests();
 S s;start(s);
 assert(s.receive(v_m1,sizeof(v_m1))==S::Reply);
 assert(s.outputLength==sizeof(v_m2)&&!memcmp(s.output,v_m2,sizeof(v_m2)));
 assert(s.receive(v_m1,sizeof(v_m1))==S::Reply); // retry M1, same SNonce
 assert(s.receive(v_m3,sizeof(v_m3))==S::Keys);
 assert(!memcmp(s.keys,v_ptk+32,16)&&!memcmp(s.keys+16,v_gtk,16)&&s.keys[32]==1);
 assert(s.outputLength==sizeof(v_m4)&&!memcmp(s.output,v_m4,sizeof(v_m4)));
 for(int i=0;i<10;++i)assert(s.receive(v_m3,sizeof(v_m3))==S::Reply); // no key reinstall
 assert(s.receive(v_retry,sizeof(v_retry))==S::Reply);
 assert(s.outputLength==sizeof(v_retry4) && !memcmp(s.output,v_retry4,sizeof(v_retry4)));
 assert(s.receive(v_m3,sizeof(v_m3))==S::Ignore); // older counter after valid retry
 assert(s.receive(v_group,sizeof(v_group))==S::Reply); // identical GTK acknowledged without reinstall
 start(s);assert(s.receive(v_m3,sizeof(v_m3))==S::Ignore);
 for(size_t n=0;n<sizeof(v_m1);++n)assert(s.receive(v_m1,n)==S::Ignore);
 for(size_t n=0;n<sizeof(v_rsn);++n)assert(!AthSecurity::supportsWPA2(v_rsn,n));
 uint8_t rsn[sizeof(v_rsn)];memcpy(rsn,v_rsn,sizeof(rsn));rsn[20]=0x40;assert(!AthSecurity::supportsWPA2(rsn,sizeof(rsn)));
 memcpy(rsn,v_rsn,sizeof(rsn));rsn[7]=2;assert(!AthSecurity::supportsWPA2(rsn,sizeof(rsn))); // TKIP group
 for(unsigned offset:{19u,31u,95u,125u}){
  start(s);assert(s.receive(v_m1,sizeof(v_m1))==S::Reply);
  std::vector<uint8_t> p(v_m3,v_m3+sizeof(v_m3));p[offset]^=1;
  auto result=s.receive(p.data(),p.size());assert(result==S::Ignore||result==S::Fatal);
 }
 start(s);assert(s.receive(v_m1,sizeof(v_m1))==S::Reply);assert(s.receive(v_bad_rsn,sizeof(v_bad_rsn))==S::Fatal);
 uint8_t badPMK[32]{};assert(s.begin(badPMK,v_snonce,v_sta,v_ap,v_rsn,sizeof(v_rsn)));
 assert(s.receive(v_m1,sizeof(v_m1))==S::Reply);assert(s.receive(v_m3,sizeof(v_m3))==S::Fatal);
 s.reset();assert(s.receive(v_m1,sizeof(v_m1))==S::Ignore);
}
