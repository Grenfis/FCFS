#!/bin/bash

for i in $(seq $2); do
	mkdir $1/$i
done
