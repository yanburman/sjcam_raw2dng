#!/usr/bin/env python

import sys

buf = open('README.md', 'r').read().replace('\r\n', '\n').replace('</br>', '').replace('<br/>', '')
if sys.platform.startswith('win'):
    buf = buf.replace('\n', '\r\n')

open('README.txt', 'w').write(buf)

