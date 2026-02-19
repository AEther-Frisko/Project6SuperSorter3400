FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# install necessary stuff
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    curl \
    unixodbc \
    unixodbc-dev \
    ca-certificates \
    gnupg \
    lsb-release

# add ODBC driver
RUN curl https://packages.microsoft.com/keys/microsoft.asc | apt-key add - \
    && curl https://packages.microsoft.com/config/ubuntu/22.04/prod.list \
       > /etc/apt/sources.list.d/mssql-release.list

RUN apt-get update \
    && ACCEPT_EULA=Y apt-get install -y msodbcsql18


WORKDIR /app

COPY . .

RUN mkdir build && cd build && \
    cmake .. && \
    cmake --build .

EXPOSE 18080

# run from project root!
CMD ["./build/server"]