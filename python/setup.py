#!/usr/bin/env python3 -u

import os
import stat
from setuptools import setup


VERSION = "0.2"


# Get an array of all scripts in a directory
def list_scripts(directory):
    scripts = []
    executable = stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH
    for file in os.listdir(directory):
        filename = os.path.join(directory, file)
        if os.path.isfile(filename):
            st = os.stat(filename)
            mode = st.st_mode
            if mode & executable:
                scripts.append(filename)
    return scripts


setup(
    name='hmtl',
    version=VERSION,
    description='HMTL python library and scripts',
    long_description="Python libraries and scripts to interact with HMTL modules",
    url='https://github.com/aphelps/HTML',
    author='Adam Phelps',
    author_email='amp@cs.stanford.edu',
    license='MIT',

    packages=['hmtl'],

    scripts=list_scripts("bin"),

    install_requires=[
      'pyserial',
      'colorama',
    ],

    setup_requires=['pytest-runner'],
    tests_require=['pytest'],

    zip_safe=False
)

if __name__ == '__main__':
    print("TEST: %s" % list_scripts("bin"))
