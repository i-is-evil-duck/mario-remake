FROM emscripten/emsdk:3.1.74 AS build

RUN apt-get update && \
    apt-get install -y ninja-build wget && \
    wget -q https://github.com/Kitware/CMake/releases/download/v3.31.6/cmake-3.31.6-linux-x86_64.tar.gz && \
    tar xzf cmake-3.31.6-linux-x86_64.tar.gz --strip-components=1 -C /usr/local && \
    rm cmake-3.31.6-linux-x86_64.tar.gz && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN emcmake cmake -B build_wasm -G Ninja -DCMAKE_BUILD_TYPE=Release && \
    emmake ninja -C build_wasm -j$(nproc)

FROM nginx:stable-alpine

COPY nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=build /src/build_wasm/bin/ /usr/share/nginx/html/

EXPOSE 80
