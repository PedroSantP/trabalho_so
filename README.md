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

## Resumo
O firmware é o primeiro software executado em um computador e faz a ponte entre o hardware e o sistema operacional; no caso do UEFI, que substituiu a BIOS tradicional, essa camada passou a oferecer inicialização modular, serviços de runtime e mecanismos de segurança como o Secure Boot, mas também se tornou alvo de ameaças persistentes por rodar com privilégios elevados antes das proteções do SO. Este trabalho examina a arquitetura do UEFI e a cadeia de confiança do Secure Boot, analisando as interfaces entre firmware e SO (serviços de runtime, variáveis NVRAM e SMM) e demonstrando, em ambiente controlado, uma vulnerabilidade da classe LogoFAIL replicada por meio de engenharia reversa e vulnerability injection, com o objetivo de evidenciar como falhas no firmware permitem contornar o Secure Boot e obter persistência, discutindo-se ainda mitigações como atualizações de firmware, listas de revogação (DBX) e boot medido com TPM. A segurança do sistema operacional depende, portanto, da integridade do firmware, que deve ser tratado como parte da superfície de ataque.

## Introdução
**Motivação:** Sistemas operacionais costumam ser tratados como a camada fundamental de software de um computador, responsável por gerenciar processos, memória e dispositivos. Esse modelo omite o fato de que, antes da execução do primeiro código do kernel, o hardware já foi inicializado e configurado por outro software: o firmware. Nos computadores modernos, esse papel é desempenhado pelo UEFI (Unified Extensible Firmware Interface), que substituiu a BIOS tradicional e passou a oferecer inicialização modular, serviços de runtime e mecanismos de segurança como o Secure Boot, projetado para manter uma cadeia de confiança entre o firmware e o sistema operacional [1][2]. 
 
Essa posição privilegiada torna o firmware relevante para a segurança. Ele executa antes de qualquer proteção do SO, com altos privilégios, e continua acessível durante a execução por meio de interfaces como serviços de runtime, variáveis NVRAM e SMM. Uma falha nesse nível pode neutralizar controles que o sistema operacional não consegue verificar nem remover por conta própria. Esse risco já se concretizou em ataques reais, como o LoJax (2018), primeiro implante de firmware UEFI observado em campanhas reais [3], o BlackLotus (2022–2023) e o LogoFAIL (2023). O trabalho parte desse contexto e se concentra em dois pontos: a cadeia de confiança do Secure Boot e as vulnerabilidades que permitem contorná-la a partir de um sistema em execução.

**Revisão bibliográfica:** A literatura acadêmica sobre o tema ainda é relativamente recente. Bashun et al. [1] já alertavam em 2013 que o UEFI, embora trouxesse mecanismos de segurança como o Secure Boot, ampliava a superfície de ataque em relação à BIOS, e apresentaram uma análise de ameaças e recomendações de proteção. Surve et al. [2] produziram uma sistematização de conhecimento (SoK) sobre a segurança do UEFI, que descreve sua arquitetura e cadeia de suprimentos, mapeia ataques reais e propõe uma taxonomia semelhante ao MITRE ATT&CK para o contexto UEFI. Os autores observam ainda que o campo permanece pouco explorado apesar do aumento de ataques.

Os casos específicos são documentados sobretudo por pesquisa de indústria. A ESET [3] analisou o BlackLotus, um bootkit vendido em fóruns por cerca de US$ 5.000 desde outubro de 2022, capaz de executar em sistemas Windows 11 totalmente atualizados com Secure Boot ativo. Ele explora a CVE-2022-21894 (Baton Drop) e, como os binários vulneráveis (embora corrigidos) não haviam sido incluídos na lista de revogação DBX, o ataque permanecia viável. A NSA [4] publicou em 2023 um guia de mitigação que reforça esse ponto e recomenda monitorar a integridade da partição de boot e, opcionalmente, personalizar a política do Secure Boot. O guia observa que o BlackLotus atua nas etapas iniciais do boot e não constitui uma ameaça de firmware no sentido estrito.

No caso do LogoFAIL, a Binarly [5] descreveu um conjunto de vulnerabilidades em bibliotecas de processamento de imagens usadas pelo firmware para exibir logotipos durante o boot. Essas bibliotecas são encontradas nos códigos de referência de três grandes fornecedores independentes de BIOS (AMI, Insyde e Phoenix), e o problema afeta dispositivos x86 e ARM. Como o parsing ocorre antes que mecanismos como Secure Boot possam intervir, uma imagem maliciosa colocada na partição EFI (ESP) pode levar à execução de código. Por fim, Matrosov e Pagani [6] apresentaram em 2025 um censo de módulos UEFI assinados e uma taxonomia de ataques ao Secure Boot, mostrando que um firmware confia, em média, em cerca de 1.500 módulos, e que uma vulnerabilidade em qualquer um deles pode comprometer a cadeia de confiança.

**Objetivos:** analisar a relação entre firmware UEFI e sistemas operacionais sob a perspectiva da segurança, demonstrando na prática como falhas nessa camada comprometem proteções do SO.

- Descrever a arquitetura do UEFI, suas fases de inicialização e o funcionamento da cadeia de confiança do Secure Boot (PK, KEK, db e dbx).
- Mapear as interfaces entre firmware e SO, como serviços de runtime, variáveis NVRAM, ACPI e SMM.
- Reproduzir, em ambiente controlado, uma vulnerabilidade conhecida de firmware (LogoFAIL) a partir de um sistema em execução.
- Discutir mitigações (atualização de firmware, revogação via DBX, boot medido com TPM) e suas limitações.

## Metodologia
**Conjunto de evidências:** Por se tratar de um trabalho de segurança de sistemas e não de um projeto orientado a dados, não há uma base de dados tradicional a coletar. O “conjunto de evidências” é composto por duas camadas:

- Material secundário, já levantado na revisão bibliográfica: artigos acadêmicos (SoK sobre UEFI, trabalho de Bashun et al.), relatórios técnicos de fornecedores de segurança (ESET, Binarly) e o guia de mitigação da NSA, além de identificadores públicos de vulnerabilidade (CVE-2022-21894) e advisórios do CERT/CC.
- Material primário, produzido durante o próprio trabalho: logs de boot, saídas de ferramentas de inspeção de firmware (variáveis UEFI, estado do Secure Boot, listas PK/KEK/db/dbx) e evidências coletadas antes e depois da exploração da vulnerabilidade escolhida, em um sistema virtualizado.
- Uma análise exploratória, nesse caso, consiste em inspecionar o firmware-alvo (versão, fabricante do código de referência — AMI, Insyde ou Phoenix — e status de atualização) para confirmar que ele é de fato vulnerável antes da tentativa de exploração.

**Modelos:** como não há modelo preditivo, esta seção descreve o ambiente experimental:

- Virtualização: QEMU/KVM com firmware OVMF (implementação open source de UEFI), permitindo testar Secure Boot em uma máquina virtual sem expor hardware físico a riscos. Snapshots da VM isolam o estado antes e depois do ataque.
- Ferramentas de inspeção: CHIPSEC, framework para análise de segurança de plataformas PC (BIOS/UEFI) mantido atualmente pelo repositório chipsec/chipsec, usado para consultar variáveis Secure Boot e rodar módulos de teste de segurança. UEFITool, para inspecionar a imagem de firmware. Ghidra, para engenharia reversa e análise estática de código binário.
- Prova de conceito: reprodução de um exploit documentado publicamente (LogoFAIL), sem desenvolvimento de exploit próprio nem uso contra sistemas de terceiros.
- Documentação: registro de comandos, capturas de tela e logs em um repositório do trabalho, para permitir a reprodução dos resultados.
Métricas

**Qualitativas:**: clareza da explicação da arquitetura UEFI e da cadeia de confiança do Secure Boot; capacidade de relacionar cada vetor de ataque pré-BDS ao mecanismo específico do Secure Boot que ele contorna; fidelidade da vulnerabilidade replicada em relação à causa raiz da classe LogoFAIL original.

**Quantitativas:**: status do Secure Boot (ativo/contornado) antes e depois da exploração, verificado via CHIPSEC ou variáveis UEFI; número de entradas em PK, KEK, db e dbx, e se houve alteração durante o ataque; número de verificações de integridade do CHIPSEC que passam antes do ataque e que falham depois; número de vetores pré-BDS mapeados por fase de boot (PEI/DXE).

**Limitações**: 
- Virtualização (OVMF) não reproduz fielmente hardware real: SPI flash, SMM completo e alguns periféricos são simplificados ou ausentes no QEMU.
- TPM é emulado via swtpm, então resultados de boot medido não refletem variações de hardware real (ex.: diferenças entre fabricantes de TPM).
- A vulnerabilidade demonstrada é uma réplica da classe LogoFAIL, não o módulo vulnerável original — os resultados caracterizam o padrão de       falha, não uma CVE específica.
- Escopo limitado a um fornecedor de código de referência (via OVMF/EDK II); não generaliza automaticamente para AMI, Insyde ou Phoenix.

**Considerações Éticas**: 
- Toda exploração ocorre em VM isolada, sem conexão com sistemas de produção ou de terceiros.
- Nenhum exploit ou binário vulnerável real é desenvolvido, distribuído ou usado contra hardware físico ou de terceiros.
- A réplica segue o princípio de *responsible disclosure*: usa apenas informações já públicas (relatório da Binarly, CVEs associadas) e não revela novas vulnerabilidades.

## Resultados
- Gráficos
- Tabelas
- Multimídia (vídeos, imagens, áudio)

## Discussão
A literatura revisada mostra uma preocupação que evolui de alerta teórico para constatação empírica: já em 2013, Bashun et al. [1] apontavam que o UEFI, apesar dos mecanismos de segurança introduzidos, ampliava a superfície de ataque em relação à BIOS; mais de uma década depois, Matrosov e Pagani [6] quantificam essa superfície ao mostrar que um firmware confia, em média, em cerca de 1.500 módulos assinados, qualquer um dos quais pode comprometer a cadeia de confiança. Essa trajetória é reforçada por Surve et al. [2], cuja sistematização de conhecimento mapeia ataques reais e propõe uma taxonomia para o campo, evidenciando que a preocupação inicial de Bashun et al. não só se confirmou como se tornou objeto de um corpo de pesquisa próprio, ainda que, segundo os autores, pouco explorado diante do crescimento dos ataques.

Os casos documentados pela indústria ilustram categorias distintas de ameaça, e essa distinção é relevante para a escolha do foco deste trabalho. O LoJax [3] e o BlackLotus [4] atuam como implante e bootkit, respectivamente, explorando falhas já corrigidas (como a CVE-2022-21894) cujo impacto persiste por ausência de revogação na DBX — a própria NSA [7] observa que o BlackLotus não constitui, em sentido estrito, uma ameaça de firmware, já que atua nas etapas iniciais do boot. O LogoFAIL [5], por sua vez, explora diretamente um parser de imagem do firmware, antes que o Secure Boot possa intervir, o que o torna mais representativo do tipo de vulnerabilidade — na camada de firmware propriamente dita — que este trabalho se propõe a investigar e justifica sua escolha como caso central da demonstração experimental.

## Conclusão
Em que medida os objetivos foram alcançados?

## Referências
BASHUN, V. et al. Threat Analysis of EFI System Firmware. In: IEEE CONFERENCE ON COMMUNICATIONS AND NETWORK SECURITY (CNS), 2013, Washington, DC. Proceedings... IEEE, 2013.

SURVE, S. et al. SoK: Systematization of Knowledge on UEFI Security. arXiv preprint arXiv:2404.14856, 2024.

ESET. LoJax: First UEFI rootkit found in the wild, courtesy of the Sednit group. [S. l.]: WeLiveSecurity, 2018. Disponível em: https://www.welivesecurity.com/2018/09/27/lojax-first-uefi-rootkit-found-wild-courtesy-sednit-group/. Acesso em: 4 nov. 2026.

ESET. BlackLotus UEFI bootkit: Myth confirmed. [S. l.]: WeLiveSecurity, 2023. Disponível em: https://www.welivesecurity.com/2023/03/01/blacklotus-uefi-bootkit-myth-confirmed/. Acesso em: 4 nov. 2026.

BINARLY. LogoFAIL. [S. l.]: Binarly, 2023. Disponível em: https://www.binarly.io/reports/logofail. Acesso em: 4 nov. 2026.

MATROSOV, A.; PAGANI, F. A Census of Signed UEFI Modules and Attack Taxonomy on Secure Boot. Cybersecurity Research, 2025.

NATIONAL SECURITY AGENCY (NSA). BlackLotus Mitigation Guide. Fort Meade: NSA Cybersecurity Information Sheet, 2023.
