FROM alpine:latest

RUN apk update \
  && apk upgrade \
  && apk add linux-headers \
  && apk add --no-cache \
    build-base

COPY . .

RUN make clean && make

CMD ["./hello_world"]

EXPOSE 3000
