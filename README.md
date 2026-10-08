# Ath10kMac

[Português (Brasil)](#português-brasil) | [English](#english)

## Português (Brasil)

Ath10kMac é uma extensão de kernel (kext) para macOS, criada para as placas Wi-Fi **Qualcomm Atheros QCA6174 e QCA9377**. O driver foi desenvolvido em C++/IOKit, usando o **ath10k do Linux** como referência para os protocolos, a inicialização do hardware e a comunicação com o firmware.

**A QCA6174 está 100% funcional no notebook utilizado nos testes, incluindo o retorno do sleep (repouso). A QCA9377 não foi testada nesta versão.**

### Compatibilidade

| Placa | Identificação PCI | Situação |
| --- | --- | --- |
| **QCA6174** | `168c:003e` | **100% funcional no notebook testado**, com conexão, tráfego em 2,4 GHz e 5 GHz e recuperação após o repouso. |
| **QCA9377** | `168c:0042` | **Não testada nesta versão.** O código e o firmware estão incluídos, mas o funcionamento ainda precisa ser confirmado em hardware. |

O projeto tem como alvo o **macOS em computadores Intel (x86_64)**. Os testes atuais foram realizados no **macOS Sequoia**. A confirmação de funcionamento da QCA6174 se refere ao notebook e às redes utilizados nos testes; outras revisões de placa e configurações ainda precisam de relatos da comunidade.

### Ath10k Connectivity é necessário

Para procurar redes e conectar ao Wi-Fi, é necessário usar o aplicativo **Ath10k Connectivity**, junto com a kext.

O aplicativo permite selecionar a rede, informar a senha, gerenciar redes conhecidas e acompanhar a conexão. A versão **1.0.0** inclui traduções em português brasileiro, inglês e espanhol.

A Ath10kMac apresenta a conexão ao macOS por meio de uma interface de rede Ethernet. A seleção das redes Wi-Fi é feita pelo **Ath10k Connectivity**, que fica na barra de menu.

### Recursos

- Busca de redes nas bandas de **2,4 GHz e 5 GHz**.
- Conexão como cliente a redes abertas e **WPA2-Personal com AES/CCMP**.
- Operação HT/VHT, incluindo **802.11n e 802.11ac**, conforme a placa e o ponto de acesso.
- Negociação de largura de canal de **20, 40 e 80 MHz**, conforme as capacidades da rede.
- Transmissão e recepção de dados, com agregação de pacotes e controle de filas.
- QoS e parâmetros **WMM/EDCA**.
- Tratamento de canais, potência e domínio regulatório.
- Recuperação da interface após o repouso, com reinicialização do hardware e reconexão pelo aplicativo.
- Informações de diagnóstico para acompanhar inicialização, firmware e estado da conexão.

O perfil desta versão é o uso como cliente Wi-Fi com redes abertas ou WPA2-Personal. **WPA3/SAE, autenticação Enterprise/EAP, AirDrop e AWDL ainda não fazem parte do suporte oferecido.** O retorno do repouso usa reinicialização e reconexão; a conexão anterior não é mantida por WoWLAN.

### Desempenho observado

Resultados informados nos testes com a **QCA6174 e a Ath10kMac 0.0.90**:

| Banda | Download | Upload |
| --- | ---: | ---: |
| **2,4 GHz** | **32,62 Mbps** | **31,23 Mbps** |
| **5 GHz** | **275,62 Mbps** | **153,03 Mbps** |

A versão **0.0.91** também foi confirmada em uso com boa velocidade e recuperação após o repouso. Os números da tabela são uma referência da máquina testada; o desempenho depende do sinal, do roteador, do canal e da conexão de internet.

### Instalação e uso

1. Obtenha a **Ath10kMac.kext** e o **Ath10k Connectivity**.
2. Instale a kext pelo método de carregamento utilizado no seu sistema e reinicie o computador.
3. Coloque o aplicativo na pasta **Aplicativos** e abra-o.
4. Use o ícone do **Ath10k Connectivity** na barra de menu para selecionar a rede e conectar.

Em sistemas que usam **OpenCore**, adicione a kext em `EFI/OC/Kexts` e registre-a em `Kernel → Add` no `config.plist`, seguindo a configuração do seu bootloader.

Para uso com o **Ath10k Connectivity 1.0.0**, utilize **macOS 13 ou posterior**. A validação de hardware descrita acima foi feita no Sequoia.

### Compilação

Requisitos:

- macOS com Xcode e as ferramentas de linha de comando.
- Python 3.
- **MacKernelSDK** na pasta `MacKernelSDK` do projeto.
- Os arquivos de firmware e de board data fornecidos com o projeto.

Na pasta do projeto, execute:

```sh
make clean
make kext
```

A kext será gerada em:

```text
build/out/Ath10kMac.kext
```

A preparação dos arquivos de firmware é realizada pelo processo de compilação. Os avisos de licença e os créditos dos componentes estão preservados nos arquivos correspondentes.

### Relatos de testes e problemas

Relatos de funcionamento em outras máquinas são bem-vindos, especialmente para a **QCA9377**.

Ao abrir uma issue, informe:

- Modelo da placa e identificação PCI.
- Versão do macOS, da kext e do Ath10k Connectivity.
- Banda utilizada e tipo de segurança da rede.
- O que aconteceu e como reproduzir.
- Se o problema ocorre ao iniciar, conectar, transferir dados ou voltar do repouso.

Se estiver usando o código-fonte, é possível coletar um diagnóstico no Terminal, a partir da pasta do projeto:

```sh
sudo sh scripts/test-runtime.sh > ~/Desktop/ath10k-runtime.txt
```

Antes de publicar o diagnóstico, remova informações pessoais, como nomes de redes, endereços MAC e endereços IP.

### Agradecimentos

Este projeto foi possível graças ao trabalho compartilhado pela comunidade:

- **[acidanthera](https://github.com/acidanthera)**, pelo **[MacKernelSDK](https://github.com/acidanthera/MacKernelSDK)**, utilizado na compilação e no desenvolvimento da kext.
- **Qualcomm / Qualcomm Atheros**, pelos drivers, firmwares e pela lógica de funcionamento do hardware que serviram de base para o projeto.
- **Linux e os desenvolvedores do [ath10k](https://wireless.docs.kernel.org/en/latest/en/users/drivers/ath10k.html)**, pelo código, documentação e lógica dos drivers. O **[ath10k do Linux v6.12](https://github.com/torvalds/linux/tree/v6.12/drivers/net/wireless/ath/ath10k)** foi a principal referência técnica das comparações.
- **[OpenIntelWireless](https://github.com/OpenIntelWireless)**, pelo **[itlwm](https://github.com/OpenIntelWireless/itlwm)** e pelo **[HeliPort](https://github.com/OpenIntelWireless/HeliPort)**, referências para drivers Wi-Fi e aplicativos de conexão no macOS.
- **[thegwchr](https://github.com/thegwchr)**, pelo **[Feixiao](https://github.com/thegwchr/Feixiao)**, estudado como referência de adaptação de drivers Wi-Fi para macOS.
- **[laobamac](https://github.com/laobamac)**, pelo **[SimpleMtkWlan](https://github.com/laobamac/SimpleMtkWlan)**, usado nas comparações de integração com o macOS, DMA, interrupções e entrega de pacotes.
- **[flany-dev](https://github.com/flany-dev)**, pelo **[RealtekRTL8822C](https://github.com/flany-dev/RealtekRTL8822C)**, referência para integração com o macOS, desempenho e recuperação da interface.
- Aos mantenedores do **[linux-firmware](https://gitlab.com/kernel-firmware/linux-firmware)** e do **[wireless-regdb](https://kernel.googlesource.com/pub/scm/linux/kernel/git/wens/wireless-regdb/)**, pelos arquivos de firmware, board data e dados regulatórios utilizados.
- **Davi**, por disponibilizar o notebook com a **QCA6174**, permitindo testar a kext em hardware real, medir o desempenho e confirmar seu funcionamento após o retorno do repouso.

Obrigado também a todos que contribuírem com testes, relatos de problemas e melhorias.

---

## English

Ath10kMac is a kernel extension (kext) for macOS, created for **Qualcomm Atheros QCA6174 and QCA9377** Wi-Fi adapters. The driver was developed in C++/IOKit, using **Linux ath10k** as a reference for protocols, hardware initialization, and firmware communication.

**The QCA6174 is 100% functional on the laptop used for testing, including recovery after sleep. The QCA9377 has not been tested with this release.**

### Compatibility

| Adapter | PCI ID | Status |
| --- | --- | --- |
| **QCA6174** | `168c:003e` | **100% functional on the tested laptop**, including connection, traffic on 2.4 GHz and 5 GHz, and recovery after sleep. |
| **QCA9377** | `168c:0042` | **Not tested with this release.** Code and firmware are included, but operation still needs to be confirmed on hardware. |

The project targets **macOS on Intel computers (x86_64)**. Current testing was performed on **macOS Sequoia**. The QCA6174 results apply to the laptop and networks used for testing; other adapter revisions and configurations still need community testing reports.

### Ath10k Connectivity is required

To scan for networks and connect to Wi-Fi, you must use the **Ath10k Connectivity** application together with the kext.

The application lets you select a network, enter its password, manage saved networks, and monitor the connection. Version **1.0.0** includes Brazilian Portuguese, English, and Spanish translations.

Ath10kMac exposes the connection to macOS through an Ethernet network interface. Wi-Fi networks are selected through **Ath10k Connectivity**, which runs in the menu bar.

### Features

- Network scanning on the **2.4 GHz and 5 GHz** bands.
- Client connections to open networks and **WPA2-Personal with AES/CCMP**.
- HT/VHT operation, including **802.11n and 802.11ac**, depending on the adapter and access point.
- **20, 40, and 80 MHz** channel width negotiation, depending on network capabilities.
- Data transmission and reception, with packet aggregation and queue management.
- QoS and **WMM/EDCA** parameters.
- Channel, transmit power, and regulatory domain handling.
- Interface recovery after sleep, with hardware reinitialization and reconnection through the application.
- Diagnostic information for initialization, firmware, and connection status.

This release supports Wi-Fi client operation with open or WPA2-Personal networks. **WPA3/SAE, Enterprise/EAP authentication, AirDrop, and AWDL are not currently supported.** Recovery after sleep uses reinitialization and reconnection; the previous connection is not retained through WoWLAN.

### Observed performance

Results reported during testing with the **QCA6174 and Ath10kMac 0.0.90**:

| Band | Download | Upload |
| --- | ---: | ---: |
| **2.4 GHz** | **32.62 Mbps** | **31.23 Mbps** |
| **5 GHz** | **275.62 Mbps** | **153.03 Mbps** |

Version **0.0.91** was also confirmed in use with good speeds and recovery after sleep. The figures in the table are a reference from the tested machine; performance depends on signal strength, the router, the channel, and the internet connection.

### Installation and usage

1. Obtain **Ath10kMac.kext** and **Ath10k Connectivity**.
2. Install the kext using your system's extension loading method and restart the computer.
3. Place the application in the **Applications** folder and open it.
4. Use the **Ath10k Connectivity** menu bar icon to select a network and connect.

On systems using **OpenCore**, add the kext to `EFI/OC/Kexts` and register it under `Kernel → Add` in `config.plist`, following your bootloader configuration.

To use **Ath10k Connectivity 1.0.0**, run **macOS 13 or later**. The hardware validation described above was performed on Sequoia.

### Building

Requirements:

- macOS with Xcode and its command line tools.
- Python 3.
- **MacKernelSDK** in the project's `MacKernelSDK` directory.
- The firmware and board data files supplied with the project.

From the project directory, run:

```sh
make clean
make kext
```

The kext will be generated at:

```text
build/out/Ath10kMac.kext
```

Firmware preparation is handled by the build process. Component license notices and credits are preserved in their respective files.

### Testing reports and issues

Reports from other machines are welcome, especially for the **QCA9377**.

When opening an issue, include:

- Adapter model and PCI ID.
- macOS, kext, and Ath10k Connectivity versions.
- Wi-Fi band and network security type.
- What happened and how to reproduce it.
- Whether the issue occurs during startup, connection, data transfers, or recovery after sleep.

If you are using the source code, you can collect diagnostics in Terminal from the project directory:

```sh
sudo sh scripts/test-runtime.sh > ~/Desktop/ath10k-runtime.txt
```

Before publishing diagnostics, remove personal information such as network names, MAC addresses, and IP addresses.

### Acknowledgments

This project was made possible by the community's shared work:

- **[acidanthera](https://github.com/acidanthera)**, for **[MacKernelSDK](https://github.com/acidanthera/MacKernelSDK)**, used to build and develop the kext.
- **Qualcomm / Qualcomm Atheros**, for the drivers, firmware, and hardware logic that served as the basis for this project.
- **Linux and the [ath10k](https://wireless.docs.kernel.org/en/latest/en/users/drivers/ath10k.html) developers**, for the driver code, documentation, and logic. **[Linux v6.12 ath10k](https://github.com/torvalds/linux/tree/v6.12/drivers/net/wireless/ath/ath10k)** was the main technical reference for the comparisons.
- **[OpenIntelWireless](https://github.com/OpenIntelWireless)**, for **[itlwm](https://github.com/OpenIntelWireless/itlwm)** and **[HeliPort](https://github.com/OpenIntelWireless/HeliPort)**, references for Wi-Fi drivers and connection applications on macOS.
- **[thegwchr](https://github.com/thegwchr)**, for **[Feixiao](https://github.com/thegwchr/Feixiao)**, studied as a reference for adapting Wi-Fi drivers to macOS.
- **[laobamac](https://github.com/laobamac)**, for **[SimpleMtkWlan](https://github.com/laobamac/SimpleMtkWlan)**, used in comparisons of macOS integration, DMA, interrupts, and packet delivery.
- **[flany-dev](https://github.com/flany-dev)**, for **[RealtekRTL8822C](https://github.com/flany-dev/RealtekRTL8822C)**, a reference for macOS integration, performance, and interface recovery.
- The maintainers of **[linux-firmware](https://gitlab.com/kernel-firmware/linux-firmware)** and **[wireless-regdb](https://kernel.googlesource.com/pub/scm/linux/kernel/git/wens/wireless-regdb/)**, for the firmware files, board data, and regulatory data used in this project.
- **Davi**, for providing the laptop with the **QCA6174**, allowing us to test the kext on real hardware, measure its performance, and confirm that it works after waking from sleep.

Thank you also to everyone who contributes testing, issue reports, and improvements.

