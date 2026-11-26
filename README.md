# INF0336 - Seminário

## Título do Seminário
Antes do Sistema Operacional: o Papel do Firmware em Plataformas Atuais

## Equipe:
202301433 - Eduardo Civitate Peuvrie Sigilião  
202306972 - Gustavo Silva Aguiar  
202501052 - Gustavo Terra Campos da Silva  
202301481 - Pedro Henrique dos Santos Pereira  

## Apresentações
1. Introdução: https://canva.link/477vgbepkuj0uq8
2. Arquitetura UEFI: https://canva.link/cmhjl7a7nr4lkny
3. Resultados Preliminares: https://canva.link/yk4cgm7h2msyi5a
4. Resultados: https://canva.link/6q8h07jktmmsdzu

## Código fonte do módulo implementado
https://github.com/gustavo-terra/logofaildemo
Este repositório será privado após o último checkpoint. Leia o README do repositório.

## Resumo

O firmware atua como ponte entre hardware e sistema operacional (SO). O UEFI substituiu a BIOS tradicional trazendo inicialização modular, serviços de runtime e Secure Boot, mas ampliou a superfície de ataque ao rodar com altos privilégios antes das proteções do SO. Este trabalho analisa a arquitetura UEFI, a cadeia de confiança do Secure Boot e as interfaces entre firmware e SO (runtime services, NVRAM e SMM). Em ambiente virtualizado, reproduz-se uma falha inspirada na classe LogoFAIL e demonstra-se o contorno da autenticação de binários EFI não assinados via manipulação do ponteiro gSecurity2. Por fim, discutem-se mitigações (atualizações, DBX e boot medido via TPM), evidenciando que a segurança do SO depende diretamente da integridade do firmware.
Introdução

## Motivação
Sistemas operacionais costumam ser vistos como a base do software, mas o hardware é inicializado antes pelo firmware UEFI (Unified Extensible Firmware Interface) [1][2]. Por executar antes do kernel, com altos privilégios, e permanecer acessível via serviços de runtime, variáveis NVRAM e SMM (System Management Mode), falhas no firmware neutralizam defesas do SO — como visto nos ataques reais LoJax (2018) [3], BlackLotus (2022–2023) [4] e LogoFAIL (2023) [5]. Este trabalho analisa a cadeia de confiança do Secure Boot e como falhas nessa camada permitem contorná-la.

## Revisão Bibliográfica:

Fundamentos e Superfície de Ataque: Bashun et al. [1] alertaram que o UEFI amplia a superfície de ataque em relação à BIOS. Surve et al. [2] sistematizaram o conhecimento (SoK) sobre UEFI, propondo uma taxonomia estilo MITRE ATT&CK. Matrosov e Pagani [6] mostraram em 2025 que um firmware confia, em média, em cerca de 1.500 módulos assinados, ampliando o risco de comprometimento.

Casos Reais: O implante LoJax [3] e o bootkit BlackLotus [4] (que explora a CVE-2022-21894) abusam de binários legítimos vulneráveis ainda não revogados na DBX. Segundo a NSA [7], o BlackLotus atua nas etapas iniciais do boot, não sendo uma falha estrita de firmware. Já o LogoFAIL [5] (documentado pela Binarly) afeta parsers de imagem de grandes fornecedores (AMI, Insyde e Phoenix) em x86 e ARM, permitindo execução de código via imagem maliciosa na partição EFI (ESP) antes da atuação do Secure Boot.

## Objetivos: Analisar a relação de segurança entre firmware UEFI e SO, demonstrando na prática o impacto de falhas nessa camada:

Descrever a arquitetura UEFI, fases de boot e a cadeia de confiança do Secure Boot (PK, KEK, db, dbx).

Mapear interfaces firmware-SO (serviços de runtime, NVRAM, ACPI e SMM).

Reproduzir aspectos da vulnerabilidade LogoFAIL em ambiente controlado.

Avaliar mitigações (atualizações, DBX, boot medido com TPM) e suas limitações.

## Metodologia

### Conjunto de Evidências:

Material Secundário: Artigos acadêmicos [1][2][6], relatórios técnicos (ESET, Binarly) [3][4][5], guia da NSA [7] e registros de CVEs [8].

Material Primário: Logs de boot, variáveis UEFI, chaves do Secure Boot (PK, KEK, db, dbx) e saídas coletadas antes e depois do experimento em VM.

### Ambiente Experimental e Ferramentas:

Virtualização: QEMU/KVM com firmware OVMF (UEFI open source) e snapshots para isolar estados antes e depois do ataque.

Análise e Inspeção: CHIPSEC (verificação de variáveis e integridade do Secure Boot), UEFITool (inspeção de imagens de firmware) e Ghidra (engenharia reversa e análise estática).

Prova de Conceito e Registro: Reprodução didática baseada no LogoFAIL (sem exploit funcional contra terceiros), com comandos e logs versionados.

### Métricas de Avaliação:

Qualitativas: Clareza na correlação entre fases do boot, vetores pré-BDS e falhas na cadeia de confiança; fidelidade conceitual à causa raiz do LogoFAIL.

Quantitativas: Estado do Secure Boot (ativo/contornado) antes e depois; contagem e integridade das chaves (PK, KEK, db, dbx); verificações do CHIPSEC aprovadas/reprovadas; vetores pré-BDS mapeados nas fases PEI e DXE.

### Limitações e Ética: O ambiente virtualizado simplifica componentes reais (SPI flash, SMM e TPM emulado via swtpm) e baseia-se no EDK II/OVMF, não generalizando automaticamente para AMI, Insyde ou Phoenix. Todos os testes ocorreram em VM isolada, seguindo princípios de responsible disclosure com dados exclusivamente públicos.
Resultados

Foi desenvolvido um módulo experimental para UEFI/EDK II inspirado nas pesquisas da Binarly sobre falhas em parsers de imagem (classe LogoFAIL), adaptado ao ambiente virtualizado do seminário:

Simulação do Desvio de Fluxo: O módulo reproduz as condições da falha no processamento de imagem, mas o redirecionamento de execução foi simulado por uma chamada interna a uma função auxiliar, sem implementar corrupção de memória arbitrária completa.

Contorno do Secure Boot (gSecurity2): De forma complementar, demonstrou-se a execução de um binário EFI não assinado mesmo com o Secure Boot reportando status ativo. A manipulação do ponteiro global gSecurity2 — responsável pelo fluxo de autenticação de imagens UEFI — provou que o estado aparente do Secure Boot não garante a integridade da validação caso estruturas internas de memória do firmware sejam comprometidas.

Escopo e Divulgação: O protótipo não constitui um exploit completo, mas valida didacticamente os mecanismos da falha e o impacto da manipulação de gSecurity2. Por segurança (responsible disclosure), o código público omitirá detalhes operacionais que permitam exploração direta.

## Discussão

A evolução da literatura confirma que a modularidade do UEFI ampliou a superfície de ataque [1][2][6]. Enquanto ameaças como LoJax [3] e BlackLotus [4] dependem de binários já assinados e falhas na atualização da DBX [7], o LogoFAIL [5] atinge diretamente o firmware antes da transferência de controle ao carregador do SO.

Na fase DXE, o firmware processa arquivos externos (como logotipos) que não passam pela mesma validação de assinatura aplicada a executáveis pelo Secure Boot. A análise estática no Ghidra e o estudo de casos reais — como as falhas de decodificação de imagens BMP CVE-2023-5058 (escrita fora dos limites por erro de alocação no módulo SystemImageDecoderDxe da Phoenix) e CVE-2023-40238 (corrupção de memória na decodificação RLE8 em firmware Insyde) [8] — confirmam que dados não confiáveis interpretados por componentes privilegiados representam um vetor crítico de comprometimento.
Conclusão

O trabalho atingiu seus objetivos ao demonstrar, teórica e experimentalmente, como falhas na camada de firmware comprometem a segurança do sistema operacional. Os testes em diferentes cenários do EDK II e configurações do Secure Boot evidenciaram que a eficácia das defesas depende tanto da implementação quanto da integridade da memória em tempo de boot.

A reprodução conceitual do LogoFAIL e a execução de binários não assinados via manipulação do ponteiro gSecurity2 comprovaram que o status ativo do Secure Boot pode ser subvertido internamente. Embora medidas como atualizações de firmware, listas de revogação (DBX) e boot medido via TPM sejam essenciais, elas possuem limitações operacionais. Conclui-se que a proteção do sistema operacional exige tratar o firmware como parte crítica da superfície de ataque.

## Conclusão
Os objetivos do trabalho foram atingidos de forma satisfatória. Foi possível analisar a relação entre firmware UEFI e sistema operacional sob a perspectiva da segurança, abordando a arquitetura de inicialização, a cadeia de confiança do Secure Boot e as principais interfaces entre firmware e sistema operacional.

Na parte experimental, foram testados diferentes cenários, incluindo versões distintas do EDK II, configurações e conjuntos de chaves do Secure Boot. Essa variação permitiu observar como diferenças de implementação e configuração influenciam o comportamento dos mecanismos de segurança, ampliando a análise para além de um único ambiente de teste.

Também foi possível reproduzir aspectos da vulnerabilidade LogoFAIL em ambiente controlado e demonstrar experimentalmente o impacto da manipulação de mecanismos internos de autenticação UEFI, com a execução de um binário EFI não assinado mesmo com o Secure Boot indicando estado habilitado. Embora não tenha sido implementada uma cadeia completa de exploração da LogoFAIL, os experimentos permitiram compreender e demonstrar como o comprometimento do firmware pode afetar proteções que se estendem ao processo de inicialização do sistema operacional.

Por fim, foram discutidas medidas de mitigação, como atualizações de firmware, revogação de imagens por meio da DBX e boot medido com TPM, considerando também suas limitações. Dessa forma, o trabalho alcançou seu objetivo principal: demonstrar que a segurança do sistema operacional depende também da integridade e da confiabilidade das camadas de firmware que antecedem sua inicialização.

## Referências
BASHUN, V. et al. Threat Analysis of EFI System Firmware. In: IEEE CONFERENCE ON COMMUNICATIONS AND NETWORK SECURITY (CNS), 2013, Washington, DC. Proceedings... IEEE, 2013.

SURVE, P. P. et al. SoK: Security Below the OS – A Security Analysis of UEFI. arXiv preprint arXiv:2311.03809, 2023. Disponível em: https://arxiv.org/abs/2311.03809. Acesso em: 4 nov. 2026.

ESET. LoJax: First UEFI rootkit found in the wild, courtesy of the Sednit group. [S. l.]: WeLiveSecurity, 2018. Disponível em: https://www.welivesecurity.com/2018/09/27/lojax-first-uefi-rootkit-found-wild-courtesy-sednit-group/. Acesso em: 4 nov. 2026.

ESET. BlackLotus UEFI bootkit: Myth confirmed. [S. l.]: WeLiveSecurity, 2023. Disponível em: https://www.welivesecurity.com/2023/03/01/blacklotus-uefi-bootkit-myth-confirmed/. Acesso em: 4 nov. 2026.

BINARLY. LogoFAIL. [S. l.]: Binarly, 2023. Disponível em: https://www.binarly.io/reports/logofail. Acesso em: 4 nov. 2026.

MATROSOV, A.; PAGANI, F. A Census of Signed UEFI Modules and Attack Taxonomy on Secure Boot. Cybersecurity Research, 2025.

NATIONAL SECURITY AGENCY (NSA). BlackLotus Mitigation Guide. Fort Meade: NSA Cybersecurity Information Sheet, 2023.

THE MALWARE GUARDIAN. Exploitation Technique: UEFI Secure Boot Bypass via gSecurity2 Corruption. GitHub. Disponível em: https://github.com/TheMalwareGuardian/Exploitation-Technique-UEFI-SecureBoot-Bypass-gSecurity2-Corruption.
