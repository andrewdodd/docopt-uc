#!/usr/bin/env python
# -*- coding: utf-8 -*-

from setuptools import setup, find_packages

setup(
    name="docopt-uc",
    description="A docopt library suitable for microcontrollers.",
    license="MIT",
    version="1.0.1",
    author="Andrew Dodd",
    author_email="andrew.john.dodd@gmail.com",
    maintainer="Andrew Dodd",
    maintainer_email="andrew.john.dodd@gmail.com",
    keywords=["docopt", "microcontroller", "cli"],
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    package_data={'docopt_uc': ['templates/*.c', 'templates/*.h']},
    zip_safe=False,
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Natural Language :: English",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.3",
        "Programming Language :: Python :: 3.4",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
    ],
    install_requires=["docopt", "jinja2"],
    entry_points={'console_scripts': [
        'docopt-uc = docopt_uc.docopt_uc:main',
    ]})
