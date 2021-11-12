.. _sasldevinstallguide:

===========================
Cyrus SASL Developer Guide
===========================

.. todo:
    This is all available at http://www.cyrusimap.org/docs/cyrus-sasl/2.1.25/install.php

Compile from source
===================

Fetch the source
-----------------

Fetch from git
##############

If you're not familiar with Git, there are detailed instructions in the :ref:`Cyrus IMAPd GitHub guide <cyrusimap:github-guide>`.

Cyrus SASL is at https://github.com/cyrusimap/cyrus-sasl

To contribute, we recommend `forking the code <https://help.github.com/articles/fork-a-repo/>`_ then issuing a pull request when you're ready.

Once forked on GitHub, you can obtain a copy by::

        git clone https://github.com/YOUR-USERNAME/REPOSITORY-NAME.git

You will then want to set your local copy to get its changes from the original repository, so it stays in sync. Use ``git remote -v`` to show the current origins of your clone which will currently be your fork::

        git remote -v
        origin  https://github.com/YOUR_USERNAME/YOUR_FORK.git (fetch)
        origin  https://github.com/YOUR_USERNAME/YOUR_FORK.git (push)

We want to set that instead to point to the primary original upstream repository::

        git remote add upstream https://github.com/cyrusimap/cyrus-sasl.git

Now we can check to see that the upstream is set::

        git remote -v
        origin    https://github.com/YOUR_USERNAME/YOUR_FORK.git (fetch)
        origin    https://github.com/YOUR_USERNAME/YOUR_FORK.git (push)
        upstream  https://github.com/cyrusimap/cyrus-sasl.git (fetch)
        upstream  https://github.com/cyrusimap/cyrus-sasl.git (push)

We recommend you create a topic branch and make your changes (don't forget to test!). Using a topic branch means you can keep your master
source in sync without affecting your changes. It also means that if your patch undergoes further revisions before inclusion, you
can easily do so.


Unpack from tarball
###################

Cyrus SASL releases are available to download in tarball format from
https://github.com/cyrusimap/cyrus-sasl/releases

Dependencies
------------

.. todo:
    ?? Libraries

Compiling
---------

::
    cd (source directory)
    sh SMakefile
    ./configure
    make
    make install
    ln -s /usr/local/lib/sasl2 /usr/lib/sasl2


Configuration
-------------

`./configure ...`

.. note:
    If you tweak configure.ac or any of the .m4 files, you will have to delete configure and then compile again to create a new configure script.
