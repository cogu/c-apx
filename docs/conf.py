# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import shutil
import subprocess

# -- Project information -----------------------------------------------------

project = 'c-apx'
copyright = '2026, Conny Gustafsson'
author = 'Conny Gustafsson'
release = '0.3.18'

# -- General configuration ---------------------------------------------------

templates_path = ['_templates']

extensions = [
    'sphinx_design',
    'sphinx.ext.githubpages',
    'sphinxcontrib.mermaid',
    'sphinxcontrib.plantuml',
    'breathe',
]

primary_domain = 'c'

# -- Breathe configuration ---------------------------------------------------

breathe_projects = {
    'c-apx': '_build/doxygen/xml',
}
breathe_default_project = 'c-apx'
breathe_domain_by_extension = {
    'h': 'c',
}

# Run Doxygen automatically if installed
docs_dir = os.path.abspath(os.path.dirname(__file__))
doxyfile_path = os.path.join(docs_dir, 'Doxyfile')
doxygen_cmd = shutil.which('doxygen')

# Also check for local virtualenv binary if not in system PATH
if not doxygen_cmd:
    venv_doxygen = os.path.abspath(os.path.join(docs_dir, '..', '.venv', 'bin', 'doxygen'))
    if os.path.exists(venv_doxygen):
        doxygen_cmd = venv_doxygen

if doxygen_cmd and os.path.exists(doxyfile_path):
    os.makedirs(os.path.join(docs_dir, '_build', 'doxygen'), exist_ok=True)
    subprocess.run([doxygen_cmd, 'Doxyfile'], cwd=docs_dir, check=False)

exclude_patterns = [
    '_build',
    '.venv',
    'README.md',
    'requirements*.txt'
]

# -- Options for HTML output -------------------------------------------------

html_theme = 'furo'
html_title = f"{project} {release} documentation"
html_static_path = ['_static']
html_css_files = [
    'custom.css',
]

source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

# -- Options for PlantUML ----------------------------------------------------

if plantuml_jar := os.environ.get('PLANTUML_JAR'):
    plantuml = f'java -jar {plantuml_jar}'
elif os.path.exists('/tmp/plantuml.jar'):
    plantuml = 'java -jar /tmp/plantuml.jar'
elif (user_plantuml := os.path.expanduser('~/plantuml/plantuml.jar')) and os.path.exists(user_plantuml):
    plantuml = f'java -jar {user_plantuml}'
elif os.path.exists('/usr/share/plantuml/plantuml.jar'):
    plantuml = 'java -jar /usr/share/plantuml/plantuml.jar'
elif shutil.which('plantuml'):
    plantuml = shutil.which('plantuml')
else:
    raise RuntimeError(
        "PLANTUML_JAR environment variable not set, and plantuml was not found in standard paths (/tmp/plantuml.jar, ~/plantuml/plantuml.jar)."
    )

plantuml_output_format = 'svg_img'
