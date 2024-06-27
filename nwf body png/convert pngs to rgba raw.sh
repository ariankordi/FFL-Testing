#!/bin/sh

for a in *.png; do
	ffmpeg -i "$a" -f rawvideo -pix_fmt rgba "$a-decoded-rgba-raw"
done
