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

## Resumo
- O firmware é o primeiro software executado em um computador e faz a ponte entre o hardware e o sistema operacional. O UEFI, que substituiu a BIOS, oferece inicialização modular, serviços de runtime e o Secure Boot. Por rodar com privilégios elevados antes das proteções do SO, o firmware tornou-se alvo de ameaças persistentes.
- Este trabalho examina a arquitetura do UEFI e a cadeia de confiança do Secure Boot. São analisadas as interfaces entre firmware e SO, como serviços de runtime, variáveis NVRAM e SMM. Uma demonstração prática, em ambiente controlado, explora uma vulnerabilidade conhecida, como LogoFail ou BlackLotus, a partir de um sistema em execução.
- Busca-se evidenciar como o firmware influencia a segurança e o funcionamento do sistema operacional. A demonstração deve mostrar que falhas em componentes do firmware permitem contornar o Secure Boot e obter persistência. Discutem-se, ainda, mitigações como atualizações de firmware, listas de revogação (DBX) e boot medido com TPM.
- A segurança do sistema operacional depende da integridade do firmware, que deve ser tratado como parte da superfície de ataque.

## Introdução
**Motivação:** Sistemas operacionais costumam ser tratados como a camada fundamental de software de um computador, responsável por gerenciar processos, memória e dispositivos. Esse modelo omite o fato de que, antes da execução do primeiro código do kernel, o hardware já foi inicializado e configurado por outro software: o firmware. Nos computadores modernos, esse papel é desempenhado pelo UEFI (Unified Extensible Firmware Interface), que substituiu a BIOS tradicional e passou a oferecer inicialização modular, serviços de runtime e mecanismos de segurança como o Secure Boot, projetado para manter uma cadeia de confiança entre o firmware e o sistema operacional [1][2]. 
 
Essa posição privilegiada torna o firmware relevante para a segurança. Ele executa antes de qualquer proteção do SO, com altos privilégios, e continua acessível durante a execução por meio de interfaces como serviços de runtime, variáveis NVRAM e SMM. Uma falha nesse nível pode neutralizar controles que o sistema operacional não consegue verificar nem remover por conta própria. Esse risco já se concretizou em ataques reais, como o LoJax (2018), primeiro implante de firmware UEFI observado em campanhas reais [3], o BlackLotus (2022–2023) e o LogoFAIL (2023). O trabalho parte desse contexto e se concentra em dois pontos: a cadeia de confiança do Secure Boot e as vulnerabilidades que permitem contorná-la a partir de um sistema em execução.

**Revisão bibliográfica:** A literatura acadêmica sobre o tema ainda é relativamente recente. Bashun et al. [1] já alertavam em 2013 que o UEFI, embora trouxesse mecanismos de segurança como o Secure Boot, ampliava a superfície de ataque em relação à BIOS, e apresentaram uma análise de ameaças e recomendações de proteção. Surve et al. [2] produziram uma sistematização de conhecimento (SoK) sobre a segurança do UEFI, que descreve sua arquitetura e cadeia de suprimentos, mapeia ataques reais e propõe uma taxonomia semelhante ao MITRE ATT&CK para o contexto UEFI. Os autores observam ainda que o campo permanece pouco explorado apesar do aumento de ataques.

Os casos específicos são documentados sobretudo por pesquisa de indústria. A ESET [3] analisou o BlackLotus, um bootkit vendido em fóruns por cerca de US$ 5.000 desde outubro de 2022, capaz de executar em sistemas Windows 11 totalmente atualizados com Secure Boot ativo. Ele explora a CVE-2022-21894 (Baton Drop) e, como os binários vulneráveis (embora corrigidos) não haviam sido incluídos na lista de revogação DBX, o ataque permanecia viável. A NSA [4] publicou em 2023 um guia de mitigação que reforça esse ponto e recomenda monitorar a integridade da partição de boot e, opcionalmente, personalizar a política do Secure Boot. O guia observa que o BlackLotus atua nas etapas iniciais do boot e não constitui uma ameaça de firmware no sentido estrito.

No caso do LogoFAIL, a Binarly [5] descreveu um conjunto de vulnerabilidades em bibliotecas de processamento de imagens usadas pelo firmware para exibir logotipos durante o boot. Essas bibliotecas são encontradas nos códigos de referência de três grandes fornecedores independentes de BIOS (AMI, Insyde e Phoenix), e o problema afeta dispositivos x86 e ARM. Como o parsing ocorre antes que mecanismos como Secure Boot possam intervir, uma imagem maliciosa colocada na partição EFI (ESP) pode levar à execução de código. Por fim, Matrosov e Pagani [6] apresentaram em 2025 um censo de módulos UEFI assinados e uma taxonomia de ataques ao Secure Boot, mostrando que um firmware confia, em média, em cerca de 1.500 módulos, e que uma vulnerabilidade em qualquer um deles pode comprometer a cadeia de confiança.

**Objetivos:** analisar a relação entre firmware UEFI e sistemas operacionais sob a perspectiva da segurança, demonstrando na prática como falhas nessa camada comprometem proteções do SO.

- Descrever a arquitetura do UEFI, suas fases de inicialização e o funcionamento da cadeia de confiança do Secure Boot (PK, KEK, db e dbx).
- Mapear as interfaces entre firmware e SO, como serviços de runtime, variáveis NVRAM, ACPI e SMM.
- Reproduzir, em ambiente controlado, uma vulnerabilidade conhecida de firmware ou de boot (LogoFAIL ou BlackLotus) a partir de um sistema em execução.
- Discutir mitigações (atualização de firmware, revogação via DBX, boot medido com TPM) e suas limitações.

## Metodologia
**Conjunto de evidências:** Por se tratar de um trabalho de segurança de sistemas e não de um projeto orientado a dados, não há uma base de dados tradicional a coletar. O “conjunto de evidências” é composto por duas camadas:

- Material secundário, já levantado na revisão bibliográfica: artigos acadêmicos (SoK sobre UEFI, trabalho de Bashun et al.), relatórios técnicos de fornecedores de segurança (ESET, Binarly) e o guia de mitigação da NSA, além de identificadores públicos de vulnerabilidade (CVE-2022-21894) e advisórios do CERT/CC.
- Material primário, produzido durante o próprio trabalho: logs de boot, saídas de ferramentas de inspeção de firmware (variáveis UEFI, estado do Secure Boot, listas PK/KEK/db/dbx) e evidências coletadas antes e depois da exploração da vulnerabilidade escolhida, em um sistema virtualizado.
- Uma análise exploratória, nesse caso, consiste em inspecionar o firmware-alvo (versão, fabricante do código de referência — AMI, Insyde ou Phoenix — e status de atualização) para confirmar que ele é de fato vulnerável antes da tentativa de exploração.

**Modelos:** como não há modelo preditivo, esta seção descreve o ambiente experimental:

- Virtualização: QEMU/KVM com firmware OVMF (implementação open source de UEFI), permitindo testar Secure Boot em uma máquina virtual sem expor hardware físico a riscos. Snapshots da VM isolam o estado antes e depois do ataque.
- Ferramentas de inspeção: CHIPSEC, framework para análise de segurança de plataformas PC (BIOS/UEFI) mantido atualmente pelo repositório chipsec/chipsec, usado para consultar variáveis Secure Boot e rodar módulos de teste de segurança. UEFITool, para inspecionar a imagem de firmware.
- Prova de conceito: reprodução de um exploit documentado publicamente (LogoFAIL ou BlackLotus, a decidir), sem desenvolvimento de exploit próprio nem uso contra sistemas de terceiros.
- Documentação: registro de comandos, capturas de tela e logs em um repositório do trabalho, para permitir a reprodução dos resultados.
Métricas

**Qualitativas:** clareza da explicação da arquitetura UEFI e da cadeia de confiança do Secure Boot; capacidade de relacionar a vulnerabilidade demonstrada com o mecanismo específico do Secure Boot que ela contorna.

**Quantitativas:** Status do Secure Boot (ativo/contornado) antes e depois da exploração, verificado via CHIPSEC ou variáveis UEFI; presença ou ausência do binário vulnerável na lista de revogação (DBX); número de verificações de integridade do CHIPSEC que passam antes do ataque e que falham depois.

## Resultados
- Gráficos
- Tabelas
- Multimídia (vídeos, imagens, áudio)

## Discussão
Análise com base na literatura utilizada na Introdução.

## Conclusão
Em que medida os objetivos foram alcançados?

## Referências
BASHUN, V. et al. Threat Analysis of EFI System Firmware. In: IEEE CONFERENCE ON COMMUNICATIONS AND NETWORK SECURITY (CNS), 2013, Washington, DC. Proceedings... IEEE, 2013. p. 347-355.

SURVE, S. et al. SoK: Systematization of Knowledge on UEFI Security. arXiv preprint arXiv:2404.14856, 2024.

LEVEILLE, J.-I. LoJax: First UEFI rootkit used in the wild, discovered by ESET. Bratislava: ESET Research, 2018. Disponível em: https://www.welivesecurity.com. Acesso em: 28 out. 2026.

BOUTIN, M. BlackLotus UEFI bootkit: Myth confirmed. Bratislava: ESET Research, 2023. Disponível em: https://www.welivesecurity.com. Acesso em: 28 out. 2026.

BINARLY. LogoFAIL: Security Flaws in UEFI Image Parsers. Los Angeles: Binarly Research, 2023. Disponível em: https://binarly.io/posts/logofail_security_flaws_in_uefi_image_parsers. Acesso em: 28 out. 2026.

MATROSOV, A.; PAGANI, F. A Census of Signed UEFI Modules and Attack Taxonomy on Secure Boot. Cybersecurity Research, 2025.

NATIONAL SECURITY AGENCY (NSA). BlackLotus Mitigation Guide. Fort Meade: NSA Cybersecurity Information Sheet, 2023.
