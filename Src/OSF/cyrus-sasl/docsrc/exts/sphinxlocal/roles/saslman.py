"""
    sphinxlocal.roles.saslman
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Turn :saslman: links into manpage references to the cyrus sasl doc tree

    :version: 0.1
    :author: Nicola Nye <nicolan@fastmail.com>

    :copyright: Copyright 2007-2016 by the Sphinx team, see AUTHORS.
    :license: BSD, see LICENSE for details.
"""

from sphinx.errors import SphinxError
from docutils import nodes, utils
from docutils.parsers.rst.roles import set_classes
from string import Template
import re

def setup(app):
    app.add_crossref_type('saslman', 'saslman', '%s', nodes.generated)
    return

class CyrusManExtension(SphinxError):
        category = ':saslman: error'
