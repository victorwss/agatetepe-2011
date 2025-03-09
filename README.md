# agatetepe-2011
Agatetepê server (2011)

Observação: Esse código foi feito em setembro de 2011 e coloquei aqui no GitHub (já em 2025) para manter público e arquivado. Não será alterado futuramente.

Segue o conteúdo do `README` convertido para o formato Markdown. Os e-mails originalmente citados já não existem mais.

----

Aluno: Victor Williams Stafusa da Silva &lt;e-mail-removido&gt;<br>
Professor: Prof. Daniel Batista &lt;e-mail-removido&gt;

Este é o servidor Agatetepê 1.0.

É um servidor HTTP que serve arquivos em um dado diretório (por padrão `./docroot`).

O servidor sabe servir conteúdo de acordo com os seguintes mime-types, que são determinados pela extensão do arquivo pedido:

| Tipo de arquivo: | Tipo mime:               |
| ---------------- | ------------------------ |
| `.txt`           | `text/plain`             |
| `.html`          | `text/html`              |
| `.js`            | `application/javascript` |
| `.es`            | `application/ecmascript` |
| `.json`          | `application/json`       |
| `.css`           | `text/css`               |
| `.png`           | `image/png`              |
| `.jpg`           | `image/jpeg`             |
| `.gif`           | `image/gif`              |
| `.xhtml`         | `application/xhtml+xml`  |

Os demais tipos são servidos como `application/x-unknown`.

Para compilar o servidor, basta rodar o `makefile` com o seguinte comando:
```
make
```

Para compilar o servidor em Windows usando o MinGW, basta rodar o `makefile` com o seguinte comando:
```
make makewin
```

Para limpar arquivos indesejáveis:
```
make clean
```

Ao compilar o servidor um arquivo executável `agatetepe` (ou `agatetepe.exe` no windows) irá surgir.

O programa pode ser executado de três formas distintas:

1. Ao executar este arquivo sem nenhum parâmetro, o servidor subirá na porta 50000 servindo os arquivos que estiverem na pasta "`./docroot`".

2. Para especificar a porta na qual o servidor subirá, esta deve ser passada como parâmetro de linha de comando. Por exemplo, para subir o servidor na porta 80:

    ```
    agatetepe 80
    ```

3. Para especificar em qual pasta estarão os arquivos que serão servidos, basta passar um segundo parâmetro que consiste em tal pasta. Por exemplo:

    ```
    agatetepe 80 /meus_documentos_na_web
    ```

* Caso o arquivo solicitado seja encontrado, este é servido e o status será 200.
* Caso o arquivo solicitado não seja encontrado, um erro 404 é devolvido.
* Caso o arquivo solicitado seja maior do que 50000 bytes, um erro 413 é devolvido.
* Caso a requisição não possa ser interpretada, um erro 400 é devolvido.
* Caso por algum motivo, apenas uma parte do arquivo possa ser lida, a resposta terá o status 206.

Se a pasta na qual o servidor lê os arquivos para servi-los for alterada enquanto o servidor estiver rodando, as alterações serão visíveis para o cliente.

O servidor só aceita requisições GET. Requisições com outros verbos HTTP resultarão em um erro 400, mesmo quando este deveria ser um 405. O servidor não tenta verificar erros 405.

Observaçõs:
1. Deixei para a última hora, mas mesmo assim consegui. Fiz mais da metade no último dia. Terminei nos últimos minutos.

2. Onde estou desenvolvendo não tem linux/unix, e sim o famigerado, bastante odiado Windows. Só isso já seria motivo para quebrar a cara.
Por causa disso, tive que me preocupar em portar o código linux para Windows sem quebrar a compatibilidade (no último dia, logicamente, e ainda tendo que pesquisar como fazer isso).
A parte mais difícil foi a função `fork()`, que não tem nada parecida no Windows. Acabei por implementar usando o `pthreads` para resolver isso.
Para garantir a portabilidade, foi preciso utilizar `#ifdefs` e algumas macros.

3. O exemplo usava a função `bzero()`. Esta função é considerada obsoleta, sendo recomendado o uso de `memset()` no lugar desta.

4. Falta menos de 10 minutos para terminar o prazo e não tenho programa para compactar em tar.gz (que droga). Serve zip mesmo?

Bem, acho que é só isso. Espero que goste do meu servidor agatetepê. :)
