#pragma once
#include <stdint.h>
#include <stddef.h>
namespace AthSecurity {
inline void wipe(void *p,size_t n){volatile uint8_t *b=(volatile uint8_t *)p;while(n--)*b++=0;}
inline bool equal(const uint8_t *a,const uint8_t *b,size_t n){uint8_t d=0;for(size_t i=0;i<n;++i)d|=a[i]^b[i];return d==0;}
inline uint16_t le16(const uint8_t *p){return static_cast<uint16_t>(p[0]|(uint16_t(p[1])<<8));}
inline uint16_t be16(const uint8_t *p){return static_cast<uint16_t>((uint16_t(p[0])<<8)|p[1]);}
inline void put16(uint8_t *p,uint16_t n){p[0]=static_cast<uint8_t>(n>>8);p[1]=static_cast<uint8_t>(n);}
inline uint64_t be64(const uint8_t *p){uint64_t n=0;for(unsigned i=0;i<8;++i)n=(n<<8)|p[i];return n;}
inline void copy(uint8_t *a,const uint8_t *b,size_t n){for(size_t i=0;i<n;++i)a[i]=b[i];}
// RSN IE selected by this STA: CCMP group/pairwise, PSK AKM, no PMF.
static constexpr uint8_t stationRSN[]={48,20,1,0,0,15,172,4,1,0,0,15,172,4,1,0,0,15,172,2,0,0};
inline bool suite(const uint8_t *p,uint8_t type){return !p[0]&&p[1]==15&&p[2]==172&&p[3]==type;}
// Strict bounds on every optional field; accept PSK+CCMP among advertised suites.
inline bool supportsWPA2(const uint8_t *ie,size_t n){
 if(n<20||n>257||ie[0]!=48||size_t(ie[1])+2!=n||le16(ie+2)!=1||!suite(ie+4,4))return false;
 size_t p=8;bool pair=false,psk=false;
 for(unsigned section=0;section<2;++section){
  if(n-p<2)return false;
  unsigned count=le16(ie+p);p+=2;
  if(!count||count>(n-p)/4)return false;
  for(unsigned i=0;i<count;++i,p+=4){if(suite(ie+p,section?2:4)){if(section)psk=true;else pair=true;}}
 }
 if(n>p){if(n-p<2)return false;uint16_t caps=le16(ie+p);p+=2;
  if(caps&((1u<<6)|(1u<<9)|(1u<<13)))return false; // MFPR, PeerKey, extended key ID
 }
 if(n>p){if(n-p<2)return false;
  unsigned count=le16(ie+p);p+=2;if(count>(n-p)/16)return false;p+=16*count;}
 if(n>p){if(n-p!=4)return false;p+=4;}
 return p==n&&pair&&psk;
}
}
