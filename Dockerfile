FROM alpine:3.14
RUN apk add --no-cache bash bison gcc gcompat make musl-dev ncurses
ENTRYPOINT ["bash"]
