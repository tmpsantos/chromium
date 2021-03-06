########################
Welcome to Native Client
########################

.. raw:: html

  <div id="home">
  <div class="pull-quote">To get the SDK and<br/>installation instructions<br/>
  <a href="/native-client/sdk/download.html">visit the SDK Download page</a>.
  </div>
  <div class="big-intro">

**Native Client** is a sandbox for running compiled C and C++ code in the
browser efficiently and securely, independent of the user's operating system.
**Portable Native Client** extends that technology with
architecture independence, letting developers compile their code once to run
in any website and on any architecture.

In short, Native Client brings the **performance** and **low-level control**
of native code to modern web browsers, without sacrificing the **security** and
**portability** of the web. Watch the video below for an overview of
Native Client, including its goals, how it works, and how
Portable Native Client lets developers run native compiled code on the web. 

.. Note::
  :class: note

  This site uses several examples of Native Client. For the best experience,
  consider downloading the `latest version of Chrome <http://www.google.com/chrome/>`_.
  When you come back, be sure to `check out our demos 
  <https://gonativeclient.appspot.com/demo>`_.

.. raw:: html

  </div>

  <iframe class="video" width="600" height="337"
  src="//www.youtube.com/embed/MvKEomoiKBA?rel=0" frameborder="0"></iframe>
  <div class="big-intro">
  
Two Types of Modules
====================

Native Client comes in two flavors.

* **Portable Native Client (PNaCl)**: Pronounced 'pinnacle', PNaCl runs single, portable (**pexe**) executables and is available
  in most implementations of Chrome. A translator built into Chrome
  translates the pexe into native code for the client hardware. The entire
  module is translated before any code is executed rather than as the code is
  executed. PNaCl modules can be hosted from any web server.
* **Native Client (NaCl)**: Also called traditional or non-portable Native
  Client, NaCl runs  
  architecture-dependent (**nexe**) modules, which are packaged into an
  application. At runtime, the browser decides which nexe to load based on the
  architecture of the client machine. NaCl modules must be run from the `Chrome
  Web Store (CWS) <https://chrome.google.com/webstore/category/apps>`_.
  Fortunately, work from PNaCl modules can be used to create NaCl modules. 
  
These flavors are described in more depth in `PNaCl and NaCl <nacl-and-pnacl>`_

.. raw:: html

  <div class="left-side">
  <div class="left-side-inner">
  <h2>Hello World</h2>
  <div class="big-intro">

To jump right in `take the tutorial <devguide/tutorial/tutorial-part1>`_ that walks you through a basic web 
application for Portable Native Client (PNaCl). This is a client-side 
application that uses HTML, JavaScript, and a Native Client module written in C++.

.. raw:: html

  </div>
  </div>
  </div>
  <h2>A Little More Advanced</h2>
  <div class="big-intro">

If you've already got the basics down, you're probably trying to get a real application ready for production. You're `building </devguide/devcycle/building>`_, `debugging </devguide/devcycle/debugging>`_ or `ready to distribute </devguide/distributing>`_.

.. raw:: html

  </div>

  <div class="left-side">
  <div class="left-side-inner">
  <h2>Nuts and Bolts</h2>
  <div class="big-intro">
  
You've been working on a Native Client module for a while now and you've run into an arcane problem. You may need to refer to the `PNaCl Bitcode Reference </reference/pnacl-bitcode-abi>`_ or the `Sandbox internals </sandbox_internals/index>`_.

.. raw:: html

  </div>
  </div>
  </div>

I Want to Know Everything
=========================

So, you like to read now and try later. Start with our `Technical Overview </overview>`_

.. raw:: html

  <div class="big-intro" style="clear: both;">

Send us questions, comments, and feedback:
`native-client-discuss <https://groups.google.com/forum/#!forum/native-client-discuss>`_.

.. raw:: html

  </div>
