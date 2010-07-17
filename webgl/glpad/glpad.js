            var gl;
            var vertexShader, fragmentShader, shaderProgram, vertexBuffer, indexBuffer;
            var hasVertexShaderSource = false, hasFragmentShaderSource = false, hasGeometry = false;
            var logContainer;

            function init() {
                //logging
                logContainer = document.getElementById('log');
                if (!window.console) window.console = {};
                if (!window.console.log) window.console.log = function(text) {
                    appendLog(text + "<br>");
                };


                var surface = document.getElementById('surface');
                try {
                    gl = surface.getContext('experimental-webgl');

                    gl.viewportWidth = surface.width;
                    gl.viewportHeight = surface.height;
                    gl.viewport(0, 0, gl.viewportWidth, gl.viewportHeight);

                    vertexShader = gl.createShader(gl.VERTEX_SHADER);
                    fragmentShader = gl.createShader(gl.FRAGMENT_SHADER);
                    shaderProgram = gl.createProgram();
                    gl.attachShader(shaderProgram, vertexShader);
                    gl.attachShader(shaderProgram, fragmentShader);

                    vertexBuffer = gl.createBuffer();
                    indexBuffer = gl.createBuffer();

                    // FIXME: Read background color from the DOM?
                    gl.clearColor(0.5, 0.0, 0.0, 1.0)
                    gl.clearDepth(1.0);
                    gl.enable(gl.DEPTH_TEST);
                    gl.depthFunc(gl.LEQUAL);

                } catch(e) {
                    alert('error initializing webgl: ' + e);
                    return;
                }



                //init gui
                var geometryTC = new TemplateControl('geometryCode', 'applyGeometryCode', 'geometryPreset');
                geometryTC.onapply = setGeometryCode;
                geometryTC.addTemplate('plane', '//TODO: make a plane');
                geometryTC.addTemplate('cube', '//TODO: make a cube');
                
                var vertexTC = new TemplateControl('vertexShader', 'applyVertexShader', 'vertexShaderPreset');
                vertexTC.onapply = setVertexShader;
                vertexTC.addTemplate('default',
                                     'attribute vec4 vertex;\n' +
                                     'uniform mat4 uMVMatrix;\n' +
                                     'uniform mat4 uPMatrix;\n' +
                                     'varying vec4 outColor;\n' +
                                     '\n' +
                                     'void main() {\n' +
                                     '  outColor = vec4(1,1,0,1);\n' +
                                     '  gl_Position = uPMatrix * uMVMatrix * vertex;\n' +
                                     '}\n');

                var fragmentTC = new TemplateControl('fragmentShader', 'applyFragmentShader', 'fragmentShaderPreset');
                fragmentTC.onapply = setFragmentShader;
                fragmentTC.addTemplate('default',
                                       'varying vec4 outColor;\n' +
                                       'void main() {\n' +
                                       '  gl_FragColor = outColor;\n' +
                                       '}\n');


                geometryTC.selectTemplate(0);
                vertexTC.selectTemplate(0);
                fragmentTC.selectTemplate(0);

                // Setup render function
                setInterval(render, 200);
            }

            function compileShader(shader, source) {
                gl.shaderSource(shader, source);
                gl.compileShader(shader);
                if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
                    logError("error compiling program:\n" + 
                             gl.getShaderInfoLog(shader) +
                            "\n" + source)
                    return false;
                }
                return tryRelink();
            }

            function setVertexShader(shaderText) {
                window.console.log("setVertexShader()\n" + shaderText);
                hasVertexShaderSource = (shaderText != "") 
                window.console.log("attributes: " + ExtractAttributesFromShaderSource(shaderText));
                window.console.log("uniforms: " + ExtractUniformsFromShaderSource(shaderText));
                return compileShader(vertexShader, shaderText);
            }

            function setFragmentShader(shaderText) {
                window.console.log("setFragmentShader()...");
                hasFragmentShaderSource = (shaderText != "") 
                return compileShader(fragmentShader, shaderText);
            }

            /*
              Generates geometry:
              vertices = flat array of xyz floats
              normals = flat array of xyz floats
              type = triangles, tristrip etc.
              indices = flat array of integers referencing vertex arrays
              */
            function setGeometryCode(code) {
                window.console.log("setGeometryCode()...");
                // FIXME: This is just dummy code. Should evaluate code instead

                var vertices = [
                    0.0,  1.0,  0.0,
                   -1.0, -1.0,  0.0,
                    1.0, -1.0,  0.0 ];

                // Upload data to vertex buffer
                gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
                gl.bufferData(gl.ARRAY_BUFFER, new WebGLFloatArray(vertices), gl.STATIC_DRAW);
                vertexBuffer.itemsize = 3;
                vertexBuffer.numitems = 3;

                return true;
            }

            // FIXME: Temporary utilities
            var mvMatrix;
            function loadIdentity() {
                mvMatrix = Matrix.I(4);
            }
            function multMatrix(m) {
                mvMatrix = mvMatrix.x(m);
            }
            function mvTranslate(v) {
                var m = Matrix.Translation($V([v[0], v[1], v[2]])).ensure4x4();
                multMatrix(m);
            }
            var pMatrix;
            function perspective(fovy, aspect, znear, zfar) {
                pMatrix = makePerspective(fovy, aspect, znear, zfar);
            }

            function render() {
                //window.console.log("render()");
                gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

                // FIXME: This uses some utility functions. Evaluate what we want to
                // use of external functionality:
                perspective(45, 1.0, 0.1, 100.0);
                loadIdentity();
                mvTranslate([-1.5, 0.0, -7.0]);

                
                // Render FBO
                gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
                gl.vertexAttribPointer(shaderProgram.vertexPositionAttribute, vertexBuffer.itemsize, gl.FLOAT, false, 0, 0);

                // Camera emulation
                gl.uniformMatrix4fv(shaderProgram.pMatrixUniform, false, new WebGLFloatArray(pMatrix.flatten()));
                gl.uniformMatrix4fv(shaderProgram.mvMatrixUniform, false, new WebGLFloatArray(mvMatrix.flatten()));

                //non-indexed:
                gl.drawArrays(gl.TRIANGLES, 0, vertexBuffer.numitems);
                //indexed: gl.drawElements(...)
            }


            function tryRelink() {
                if (hasVertexShaderSource && hasFragmentShaderSource) {
                    window.console.log("Linking shaders...");
                    gl.linkProgram(shaderProgram);
                    if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
                        //TODO:get the exact error
                        logError("error linking program");
                        return false;
                    }
                    

                    //FIXME: don't call this every time...
                    gl.useProgram(shaderProgram);

                    // Get reference to our vertex attributes and enable them
                    shaderProgram.vertexPositionAttribute = gl.getAttribLocation(shaderProgram, "vertex");
                    gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);
                    // Handle uniforms
                    shaderProgram.pMatrixUniform = gl.getUniformLocation(shaderProgram, "uPMatrix");
                    shaderProgram.mvMatrixUniform = gl.getUniformLocation(shaderProgram, "uMVMatrix");
                    return true;
                }
                return true;
            }

            function logError(message) {
                appendLog("<span class='error'>" + message + "</span><br>");
            }

            function logInfo(message){
                appendLog(message + "<br>");
            }

            function appendLog(htmlText) {
                logContainer.innerHTML = logContainer.innerHTML + htmlText;
            }



            function TemplateControl(textAreaId, applyButtonId, selectListId) {
                var textArea = document.getElementById(textAreaId);
                var applyButton = document.getElementById(applyButtonId);
                var selectList = document.getElementById(selectListId);
                var blockUpdate = false;
                var me = this;
                var templates = new Object();

                //add custom entry
                selectList.innerHTML = "<option value='custom'>custom</option>";
                selectList.onchange = function() {
                    var selectedName = selectList.options[selectList.selectedIndex].value;
                    if (selectedName != 'custom') {
                        var text = templates[selectedName];
                        blockUpdate = true;
                        textArea.value = text;
                        blockUpdate = false;
                        applyButton.onclick();
                    }
                };

                textArea.onkeypress = function() {
                    textArea.onchange();
                }

                textArea.onchange = function() {
                    if (!blockUpdate) {
                        applyButton.disabled = false;
                        selectList.selectedIndex = selectList.options.length-1;
                    }
                };

                applyButton.onclick = function() {
                    if (me.onapply) {
                        if (me.onapply(textArea.value))
                            applyButton.disabled = true;
                    }
                };

                this.addTemplate = function(name, script) {
                    templates[name] = script;
                    var oldSelected = selectList.selectedIndex;
                    selectList.innerHTML = "<option value='" + name + "'>" + name +"</option>" + selectList.innerHTML;
                    selectList.selectedIndex = oldSelected + 1;
                };

                this.selectTemplate = function(index) {
                    selectList.selectedIndex = index;
                    selectList.onchange();
                }
            }
