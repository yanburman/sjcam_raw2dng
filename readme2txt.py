#!/usr/bin/env python

import sys

buf = open('README.md', 'r').read().replace('\r\n', '\n').replace('</br>', '').replace('<br/>', '').replace('![Image](https://github.com/yanburman/sjcam_raw2dng/blob/master/resources/gui.png)\n\n', '')
if sys.platform.startswith('win'):
    buf = buf.replace('\n', '\r\n')

open('README.txt', 'w').write(buf)

