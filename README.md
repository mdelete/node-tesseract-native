node-tesseract-native
=====================

C++ module for node providing OCR with tesseract and leptonica

Prerequisites
-------------
 * Have linux or OSX (at least these are tested OSes)
 * Have node (>= 0.12.0) and node-gyp installed
 * Have leptonica (~1.68) libs and headers installed
 * Have tesseract (~3.02) libs and headers installed

Build
-----
Checkout the repository and build it yourself using

    node-gyp configure && node-gyp build
    
or use npm

    npm install tesseract_native

Supported Picture Formats
-------------------------

The module can handle every picture format leptonica can handle (see there), but as this module is likely to be used in an online service, pictures should be as small as possible. A 1.3 MegaPixel picture converted to B/W using adaptive threshold filtering, saved as PNG will be 50KB on average. This is were you want to go.

Test your setup
---------------

You can test your setup using the provided *test.js* script on the command-line

    $ node test.js HelloWorld.jpg

Example server
--------------

The code below shows a fully functional server where you can POST pictures to. The response will contain the recognized plain text or be empty if nothing was recognized or something went wrong.

    var tesseract = require('tesseract_native');
    var http = require('http');
    
    var server = http.createServer(function(request, response)
    {
        if(request.method === 'POST')
        {
            var totalSize = 0;
            var bufferList = new Array();
            var myOcr = new tesseract.OcrEio();
            
            request.on('data', function(data) {
                bufferList.push(data);
                totalSize += data.length;
                if (totalSize > 1e6) {
                    console.log('Request body too large');
                    request.connection.destroy();
                }
            });
            
            request.on('end', function() {
                var buffer = Buffer.concat(bufferList, totalSize);
                myOcr.ocr(buffer, function(err, result) {
                    if(err) {
                        response.writeHead(500, {'Content-Type': 'text/plain'});
                        response.end("Error " + err);
                    } else {
                        response.writeHead(200, {'Content-Type': 'text/plain'});
                        response.end(result);
                    }
                });
            });
            
        } else {
            request.connection.destroy();
        }
    }).listen(process.argv[2]);
    
Parameters
----------
    
The OCR function also accepts a config object as second and the callback as third parameter like this:

    myOcr.ocr(buffer, { lang:"eng", rect:[0,0,400,400] }, function(result) {
        // do something
    });
    
The first supported parameter is `tessdata`, which is the path to you Tesseract data directory (`/usr/local/share/tessdata/` by default). The second is `lang` which can be any three-character code for a language you have installed with Tesseract (`eng` by default). The third is `rect`, which is an array describing a rect of the form [`X`, `Y`, `WIDTH`, `HEIGHT`] limiting the image region for recognition. If you try the above rect with the provided test image it should land you in *hell*...

Why?
----

The question may arise. I've seen many tesseract wrappers for node and none of them I found did it quite right, some of them even did it wrong. The philosophy (and necessity) behind node is not to block, so everything that does work has to do it asynchronously and emit an event/execute a closure when it's done. If you don't do that, your node application will simply not perform well.

But even in this code you can see a very crude solution, performance-wise. The tesseract api is instantiated and initialized on every call to the *ocr* method. Why did't I do that when loading the module or when the constructor gets called? It has multiple reasons:

 * Simplicity: Initializing tesseract involves file system access, that means, must be performed asynchronously. The OCR work is done by adding the function to the asynchronous *uv_queue_work*, for simplicity I bundled all time-consuming tasks in the function that gets passed to the queue. So even though initializing wastes some cycles, it is still perfectly non-blocking.

 * Flexiblity: The language setting is passed on initialization, so by initializing on each request, the language used for detection can be set with each request.

 * Robustness: The tesseract context may not be thread-safe. There are hints in the tesseract code that suggest that. I will look further into it and I will be experimenting with a version that initializes the tesseract context at load time.

