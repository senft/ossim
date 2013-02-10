#!/bin/sh
git pull upstream master 
git add . 
git commit -a -m "sync upstream on `date`"
git push 
