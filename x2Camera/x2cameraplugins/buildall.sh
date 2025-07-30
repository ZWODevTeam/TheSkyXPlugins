#!/bin/sh
buildplatform(){
	rm lib/$1/*
	make clean
	make platform=$1 ver=release
}
buildplatform x86
buildplatform x64
buildplatform armv5
buildplatform armv6
buildplatform armv7
buildplatform armv8

