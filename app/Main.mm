#import <Cocoa/Cocoa.h>
#import <IOKit/IOKitLib.h>
#import <CommonCrypto/CommonHMAC.h>
#import <CommonCrypto/CommonCryptor.h>
#import <CommonCrypto/CommonKeyDerivation.h>
#import <Security/Security.h>
#include <unistd.h>
#include "../src/shared/Ath10kControl.hpp"
#include "../src/shared/Ath10kSupplicant.hpp"
struct MacCrypto {
 static bool random(uint8_t *p,size_t n){return SecRandomCopyBytes(kSecRandomDefault,n,p)==errSecSuccess;}
 static bool hmac(const uint8_t *k,size_t kl,const uint8_t *p,size_t n,uint8_t *out){CCHmac(kCCHmacAlgSHA1,k,kl,p,n,out);return true;}
 static bool decrypt(const uint8_t *key,const uint8_t *in,uint8_t *out){size_t done=0;return CCCrypt(kCCDecrypt,kCCAlgorithmAES,kCCOptionECBMode,key,16,nullptr,in,16,out,16,&done)==kCCSuccess && done==16;}
};
@interface Controller:NSObject<NSApplicationDelegate> {
 NSWindow *_window;
 NSPopUpButton *_networks;
 NSSecureTextField *_password;
 NSTextField *_status;
 NSButton *_connect,*_disconnect;
 NSMutableArray<NSData *> *_rows;
 NSTimer *_timer;
 io_connect_t _driver;
 AthSecurity::Supplicant<MacCrypto> _supplicant;
 uint8_t _pending[160];size_t _pendingLength;
 BOOL _disconnecting;
 BOOL _session,_secure,_keysSubmitted,_m4Sent,_authorized,_failed;
}
@end
@implementation Controller
-(kern_return_t)call:(uint32_t)op request:(AthControl::Request *)input response:(AthControl::Response *)output {
 AthControl::Request empty{};if(!input)input=&empty;input->operation=op;
 AthControl::Response temporary{};if(!output)output=&temporary;
 size_t size=sizeof(*output);kern_return_t r=IOConnectCallStructMethod(_driver,0,input,sizeof(*input),output,&size);
 if(r==KERN_SUCCESS && (size!=sizeof(*output)||output->version!=AthControl::version))return kIOReturnBadMessageID;
 return r;
}
-(void)fail:(NSString *)message {
 _status.stringValue=message;_failed=YES;_pendingLength=0;
 if(_session)[self call:AthControl::Abort request:nullptr response:nullptr];
 _supplicant.reset();AthSecurity::wipe(_pending,sizeof(_pending));_password.stringValue=@"";
 _connect.enabled=NO;_disconnect.enabled=NO;
}
-(void)applicationDidFinishLaunching:(NSNotification *)note {
 (void)note;_rows=[NSMutableArray array];
 _window=[[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,700,350) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];
 _window.title=@"Ath10k Connectivity — experimental";[_window center];
 NSView *v=_window.contentView;
 NSTextField *title=[NSTextField labelWithString:@"Ath10k Connectivity"];title.font=[NSFont boldSystemFontOfSize:25];title.frame=NSMakeRect(24,294,650,36);[v addSubview:title];
 NSTextField *subtitle=[NSTextField labelWithString:@"Redes abertas e WPA2-Personal (AES)"];subtitle.frame=NSMakeRect(24,265,650,24);[v addSubview:subtitle];
 _networks=[[NSPopUpButton alloc] initWithFrame:NSMakeRect(24,212,520,32) pullsDown:NO];[v addSubview:_networks];
 NSButton *refresh=[NSButton buttonWithTitle:@"Atualizar lista" target:self action:@selector(refresh:)];refresh.frame=NSMakeRect(550,212,128,32);[v addSubview:refresh];
 _password=[[NSSecureTextField alloc] initWithFrame:NSMakeRect(24,163,520,30)];_password.placeholderString=@"Senha WPA2 (8–63 bytes)";[v addSubview:_password];
 _connect=[NSButton buttonWithTitle:@"Conectar" target:self action:@selector(connect:)];_connect.frame=NSMakeRect(550,162,128,32);[v addSubview:_connect];
 _disconnect=[NSButton buttonWithTitle:@"Desconectar" target:self action:@selector(disconnect:)];_disconnect.frame=NSMakeRect(550,124,128,32);_disconnect.enabled=NO;[v addSubview:_disconnect];
 _status=[NSTextField wrappingLabelWithString:@"Procurando a kext 0.0.63…"];_status.frame=NSMakeRect(24,66,515,78);[v addSubview:_status];
 NSTextField *foot=[NSTextField wrappingLabelWithString:@"Prévia: desconecte antes de trocar de rede. Mantenha o app aberto durante a conexão. WPA3, PMF obrigatório, DFS e renovação de chaves ainda sem suporte."];foot.font=[NSFont systemFontOfSize:11];foot.frame=NSMakeRect(24,12,650,45);[v addSubview:foot];
 [_window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];
 io_service_t service=IOServiceGetMatchingService(kIOMasterPortDefault,IOServiceMatching("Ath10kPCIDevice"));
 if(!service){_status.stringValue=@"Kext não encontrada. Compile e carregue a versão 0.0.63.";_connect.enabled=NO;return;}
 kern_return_t r=IOServiceOpen(service,mach_task_self(),AthControl::clientType,&_driver);IOObjectRelease(service);
 if(r){_status.stringValue=[NSString stringWithFormat:@"Falha ao abrir a kext: 0x%x. Confirme versão e permissão.",r];_connect.enabled=NO;return;}
 [self refresh:nil];_timer=[NSTimer scheduledTimerWithTimeInterval:0.1 target:self selector:@selector(tick:) userInfo:nil repeats:YES];
}
-(void)refresh:(id)sender {
 (void)sender;if(!_driver||_session)return;
 AthControl::Response st{};if([self call:AthControl::Status request:nullptr response:&st])return;
 if(!(st.flags&2)){_status.stringValue=@"Aguardando a varredura inicial. Clique em Atualizar lista após cerca de 35 segundos.";return;}
 [_rows removeAllObjects];[_networks removeAllItems];
 for(uint32_t i=0;i<st.count;++i){AthControl::Request q{};AthControl::Response r{};q.index=i;
  if([self call:AthControl::BSS request:&q response:&r]||r.length<44)continue;
  NSData *data=[NSData dataWithBytes:r.data length:r.length];[_rows addObject:data];
  NSString *ssid=[[NSString alloc] initWithBytes:r.data+8 length:r.data[0] encoding:NSUTF8StringEncoding];
  if(!ssid.length)ssid=@"SSID oculto ou binário";
  NSString *security=r.data[1]?(r.data[41]?@"WPA2 AES":@"não suportada"):@"aberta";
  [_networks addItemWithTitle:[NSString stringWithFormat:@"%@ • %@ • canal %u • %02x:%02x:%02x:%02x:%02x:%02x",ssid,security,r.data[40],r.data[2],r.data[3],r.data[4],r.data[5],r.data[6],r.data[7]]];
 }
 _status.stringValue=@"Selecione a rede. A senha não será gravada em arquivo nem no Info.plist.";
}
-(void)connect:(id)sender {
 (void)sender;if(_session||!_rows.count||_networks.indexOfSelectedItem<0)return;
 const uint8_t *b=static_cast<const uint8_t *>(_rows[(NSUInteger)_networks.indexOfSelectedItem].bytes);
 uint8_t channel=b[40];if(channel>=52 && channel<=144){_status.stringValue=@"Nesta prévia, use um canal sem DFS: 1–13, 36–48 ou 149–165.";return;}
 if(b[1]&&!b[41]){_status.stringValue=@"Use WPA2-Personal com AES e PMF opcional. WPA3, TKIP e redes corporativas não são suportados.";return;}
 AthControl::Response status{};if([self call:AthControl::Status request:nullptr response:&status])return;
 if(!(status.flags&1)){[self fail:[NSString stringWithFormat:@"A kext parou: %s. Recarregue/reinicie antes de conectar.",status.error]];return;}
 if(!(status.flags&2)){_status.stringValue=@"A varredura inicial ainda não terminou. Aguarde e atualize a lista.";return;}
 _secure=b[1];
 if(_secure){
  NSData *password=[_password.stringValue dataUsingEncoding:NSUTF8StringEncoding];
  if(password.length<8||password.length>63){_status.stringValue=@"A senha WPA2 deve ter entre 8 e 63 bytes.";return;}
  uint8_t pmk[32]{},nonce[32]{};
  bool ok=CCKeyDerivationPBKDF(kCCPBKDF2,(const char *)password.bytes,password.length,b+8,b[0],kCCPRFHmacAlgSHA1,4096,pmk,32)==kCCSuccess;
  ok=ok && SecRandomCopyBytes(kSecRandomDefault,32,nonce)==errSecSuccess;
  ok=ok && _supplicant.begin(pmk,nonce,status.local,b+2,b+44,AthSecurity::le16(b+42));
  AthSecurity::wipe(pmk,32);AthSecurity::wipe(nonce,32);_password.stringValue=@"";
  if(!ok){_status.stringValue=@"Não foi possível iniciar a negociação WPA2.";return;}
 }
 AthControl::Request q{};q.length=8+b[0];memcpy(q.data,b,q.length);
 kern_return_t r=[self call:AthControl::Connect request:&q response:nullptr];
 if(r){
  _supplicant.reset();AthControl::Response detail{};
  kern_return_t query=[self call:AthControl::Status request:nullptr response:&detail];
  if(!query && strcmp(detail.error,"none"))_status.stringValue=[NSString stringWithFormat:@"Não iniciou: %s (0x%x). Recarregue/reinicie se o runtime estiver parado.",detail.error,r];
  else if(r==kIOReturnExclusiveAccess)_status.stringValue=@"Já existe uma sessão. Desconecte no aplicativo que iniciou a conexão.";
  else if(r==kIOReturnNotReady)_status.stringValue=@"A interface ainda não está pronta. Habilite o serviço de rede no macOS e aguarde o scan.";
  else _status.stringValue=[NSString stringWithFormat:@"Não iniciou (0x%x). Consulte o diagnóstico test-runtime.sh.",r];
  return;
 }
 _failed=NO;_keysSubmitted=NO;_m4Sent=NO;_authorized=NO;_pendingLength=0;
 _session=YES;_connect.enabled=NO;_networks.enabled=NO;_password.enabled=NO;_status.stringValue=@"Associando à rede…";
}
-(void)disconnect:(id)sender {
 (void)sender;if(!_session || _disconnecting)return;
 kern_return_t r=[self call:AthControl::Disconnect request:nullptr response:nullptr];
 if(r){_status.stringValue=[NSString stringWithFormat:@"Não foi possível iniciar a desconexão (0x%x). Aguarde a conexão terminar.",r];return;}
 _disconnecting=YES;_disconnect.enabled=NO;
 _supplicant.reset();AthSecurity::wipe(_pending,sizeof(_pending));_pendingLength=0;_password.stringValue=@"";
 _status.stringValue=@"Desconectando… aguardando a confirmação do firmware.";
}
-(void)tick:(NSTimer *)timer {
 (void)timer;if(!_driver||_failed)return;
 AthControl::Response status{};if([self call:AthControl::Status request:nullptr response:&status]){[self fail:@"Comunicação com a kext perdida."];return;}
 if(!(status.flags&1)){[self fail:[NSString stringWithFormat:@"Kext interrompida: %s. Recarregue para tentar novamente.",status.error]];return;}
 if(_disconnecting){
  if(status.flags&64)return;
  if(status.stage!=0){[self fail:@"A desconexão terminou em estado inesperado. Consulte o diagnóstico."];return;}
  _session=NO;_disconnecting=NO;_secure=NO;_keysSubmitted=NO;_m4Sent=NO;_authorized=NO;
  _connect.enabled=YES;_networks.enabled=YES;_password.enabled=YES;
  _status.stringValue=@"Desconectado. Selecione uma rede e clique em Conectar; não é necessário recarregar a kext.";
  return;
 }
 if(!_session)return;
 _disconnect.enabled=(status.flags&128) && (status.flags&4);
 if(_secure){
  for(unsigned i=0;i<4;++i){AthControl::Response rx{};
   if([self call:AthControl::ReceiveEAPOL request:nullptr response:&rx])break;
   auto result=_supplicant.receive(rx.data,rx.length);
   if(result==AthSecurity::Supplicant<MacCrypto>::Fatal){[self fail:[NSString stringWithUTF8String:_supplicant.error]];return;}
   if(result==AthSecurity::Supplicant<MacCrypto>::Keys){
    AthControl::Request keys{};keys.length=39;memcpy(keys.data,_supplicant.keys,39);
    kern_return_t r=[self call:AthControl::InstallKeys request:&keys response:nullptr];
    AthSecurity::wipe(keys.data,sizeof(keys.data));AthSecurity::wipe(_supplicant.keys,sizeof(_supplicant.keys));
    if(r){[self fail:@"A kext rejeitou a instalação de chaves."];return;}_keysSubmitted=YES;
   }
   if(result==AthSecurity::Supplicant<MacCrypto>::Keys||result==AthSecurity::Supplicant<MacCrypto>::Reply){
    memcpy(_pending,_supplicant.output,_supplicant.outputLength);_pendingLength=_supplicant.outputLength;
   }
  }
  if(_pendingLength && (!_keysSubmitted || status.phase>=3)){
   AthControl::Request tx{};tx.length=(uint32_t)_pendingLength;memcpy(tx.data,_pending,_pendingLength);
   kern_return_t r=[self call:AthControl::SendEAPOL request:&tx response:nullptr];
   if(!r){_pendingLength=0;if(_keysSubmitted)_m4Sent=YES;}
   else if(r!=kIOReturnBusy && r!=kIOReturnNotReady){[self fail:@"Falha ao enviar a resposta EAPOL."];return;}
  }
  if(_m4Sent&&!_authorized&&!(status.flags&8)&&status.phase==3){
   kern_return_t r=[self call:AthControl::Authorize request:nullptr response:nullptr];if(!r)_authorized=YES;
  }
 }
 if(status.flags&4)_status.stringValue=@"Conectado. O macOS pode solicitar o IP por DHCP. Valide o endereço e a navegação; mantenha o app aberto.";
 else if(status.flags&32)_status.stringValue=@"Uma transmissão EAPOL falhou; aguardando retransmissão do roteador…";
 else _status.stringValue=[NSString stringWithFormat:@"Conectando… associação %u, segurança %u. Aguardando confirmação do firmware.",status.stage,status.phase];
}
-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender{(void)sender;return YES;}
-(void)applicationWillTerminate:(NSNotification *)note{(void)note;[_timer invalidate];_supplicant.reset();AthSecurity::wipe(_pending,sizeof(_pending));if(_driver)IOServiceClose(_driver);}
@end
int main(int argc,const char **argv){(void)argc;(void)argv;@autoreleasepool{[NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];Controller *c=[Controller new];NSApp.delegate=c;[NSApp run];}return 0;}
