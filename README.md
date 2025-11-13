WorkWell
Sistema Inteligente de Monitoramento Ambiental para Bem-Estar no Trabalho

## Introdução

O ambiente de trabalho moderno enfrenta desafios crescentes relacionados à saúde, segurança e bem-estar dos trabalhadores. Temperaturas elevadas, baixa qualidade do ar, iluminação inadequada e umidade extrema são fatores que comprometem a produtividade e podem gerar riscos à saúde.
Contudo, grande parte desses fatores não é monitorada de forma contínua.

Para solucionar esse problema, foi desenvolvido o WorkWell, um sistema IoT baseado em ESP32, sensores ambientais e comunicação via MQTT, capaz de monitorar o ambiente em tempo real e apresentar as informações em um dashboard interativo.

O objetivo é fornecer feedback instantâneo, prevenir riscos e promover um ambiente de trabalho mais inteligente, seguro e humano.

## O Problema

Ambientes industriais, técnicos e operacionais frequentemente apresentam condições que variam ao longo do dia:

Excesso de calor

Gases nocivos no ar

Umidade inadequada

Falta de iluminação

Quando ignorados, esses fatores causam:

 Estresse térmico
 Fadiga
 Queda de produtividade
 Intoxicação leve
 Riscos graves à saúde

Empresas que não monitoram o ambiente tendem a reagir depois do problema ocorrer — o que gera custos, afastamentos e incidentes.

## A Solução: WorkWell

O WorkWell é um sistema completo composto por:

🔹 Hardware (ESP32 + sensores)

DHT22 → temperatura e umidade

MQ-2 → qualidade do ar

LDR → intensidade luminosa

Envio automático via Wi-Fi + MQTT

Totalmente reproduzido no Wokwi

## Software (Dashboard Python)

Criado em:

Flask

MQTT Client (paho-mqtt)

Socket.IO (atualização em tempo real)

O dashboard apresenta:

 Temperatura
 Umidade
 Qualidade do ar
 Luminosidade
 Mensagem de risco para saúde
 Atualização instantânea
 Interface visual simples e direta

## Arquitetura do Sistema
[Sensores] → [ESP32] → (Wi-Fi) → [MQTT Broker] → [Dashboard Python] 

## Links do Projeto

Simulação no Wokwi:
https://wokwi.com/projects/445713396446002177


Vídeo explicativo :
https://www.loom.com/share/2e203036c2914e6cbf70d1eadc67930e

## Instruções de Replicação do Projeto

Navegador 

Conta no Wokwi

Python 3.10+

Dependências:

pip install flask flask-socketio paho-mqtt eventlet

Como rodar o Dashboard Localmente

Baixe os arquivos do dashboard:
app.py
templates/index.html

Dentro da pasta, execute:

python app.py


Acesse no navegador:

http://127.0.0.1:5000


Com o dashboard aberto, inicie o projeto do Wokwi.
As métricas aparecerão automaticamente.

Como replicar no Wokwi

Acesse o link do projeto.

Clique em Fork para copiar.

Modifique o Wi-Fi se necessário.

Clique em Start Simulation.

Veja os dados no Serial Monitor + dashboard.

## Explicação Técnica – MQTT

O sistema utiliza o protocolo MQTT, ideal para IoT devido ao baixo consumo de energia e latência.

► Broker utilizado:
44.223.43.74
Porta: 1883


O ESP32 se conecta ao Wi-Fi e publica mensagens no tópico principal:

  Tópico utilizado
workwell/monitoramento

Nele o ESP32 envia um payload JSON:

{
  "temperatura_C": 28.5,
  "umidade_pct": 52,
  "mq2_raw": 750,
  "gas_nivel_pt": "Moderado",
  "ldr_raw": 1800,
  "luz_desc_pt": "Baixa",
  "ip": "10.10.0.2"
}

## Interpretação dos Sensores
   Qualidade do Ar (MQ-2)
Valor MQ2	Classificação	Risco
< 500	Boa	Sem risco
500–1500	Moderada	Atenção
> 1500	Ruim	Risco à saúde
   Luminosidade (LDR)
Valor	Interpretação
< 1000	Baixa
1000–3000	OK
> 3000	Muito alta
   Temperatura e Umidade (DHT22)

Temperatura ideal: 22°C – 28°C

Umidade ideal: 40% – 60%

## Interface do Dashboard
<img width="1846" height="898" alt="image" src="https://github.com/user-attachments/assets/8b8038f8-0d87-4e6c-96a2-565a1fd0f321" />

## Conclusão

O WorkWell demonstra como tecnologias digitais podem tornar o futuro do trabalho:

mais seguro,

mais humano,

e mais inteligente.

Com sensores de baixo custo, MQTT e um dashboard interativo, o sistema cria um ambiente preventivo, evitando situações de risco e protegendo a saúde dos profissionais.
