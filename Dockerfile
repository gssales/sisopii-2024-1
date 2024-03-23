FROM alpine:latest

RUN apk update \
  && apk upgrade \
  && apk add --no-cache \
    build-base

COPY . .

RUN make

CMD ["./hello_world"]

EXPOSE 3000
