#!/usr/bin/env python2

import os
from shutil import copyfile

src=".launch.json"
dst=".vscode/launch.json"

if os.path.exists(dst):
  os.remove(dst)

copyfile(src, dst)
