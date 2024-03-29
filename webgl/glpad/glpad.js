//to integrate new shader or geometry presets, just add the file name without extension 
//to this list and place the file in the respective directory
// the first entry of the list will be used as the default
var geometryFiles = [ 'triangle', 'plane', 'texplane', 'house', 'cube', 'icosphere' ];
var vertexShaderFiles = [ 'default', 'texture', 'vertexcolor' ];
var fragmentShaderFiles = [ 'default', 'texture' ];

var gl;
var vertexShader, fragmentShader, shaderProgram, texture;
var attributes, uniforms;
var vbo = { }
var logContainer;

function checkGLError() {
    err = gl.getError();
    if (err != gl.NO_ERROR) {
        logError('GL Error: ' + err);
    }
}

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
        texture = gl.createTexture();

        // FIXME: Read background color from the DOM?
        gl.clearColor(0.6, .7, 0.7, 1.0)
        gl.clearDepth(1.0);
        gl.enable(gl.DEPTH_TEST);
        gl.depthFunc(gl.LEQUAL);

        checkGLError();
    } catch(e) {
        alert('error initializing webgl: ' + e);
        return;
    }

    geometryFiles = geometryFiles.reverse();
    vertexShaderFiles = vertexShaderFiles.reverse();
    fragmentShaderFiles = fragmentShaderFiles.reverse();

    //init gui
    var applyButton = document.getElementById('apply');
    var geometryTC = new TemplateControl('geometryCode', 'geometryPreset');
    geometryTC.onchange = function() { applyButton.disabled = false; };

    var vertexTC = new TemplateControl('vertexShader', 'vertexShaderPreset');
    vertexTC.onchange = function() { applyButton.disabled = false; };

    var fragmentTC = new TemplateControl('fragmentShader', 'fragmentShaderPreset');
    fragmentTC.onchange = function() { applyButton.disabled = false;};

    applyButton.onclick = function() {
        clearLog();
        setVertexShader(vertexTC.getText());
        setFragmentShader(fragmentTC.getText());
        setGeometryCode(geometryTC.getText());
        if (tryRelink()) setShaderData();
        applyButton.disabled = true;
        checkGLError();
    };

    //load presets
    for(var i=0; i < geometryFiles.length; i++)
        geometryTC.addTemplate(geometryFiles[i], loadTextFile('geom/' + geometryFiles[i] + '.js'));

    for(var i=0; i < vertexShaderFiles.length; i++)
        vertexTC.addTemplate(vertexShaderFiles[i], loadTextFile('vert/' + vertexShaderFiles[i] + '.vert'));

    for(var i=0; i < fragmentShaderFiles.length; i++)
        fragmentTC.addTemplate(fragmentShaderFiles[i], loadTextFile('frag/' + fragmentShaderFiles[i] + '.frag'));

    //select presets
    vertexTC.selectTemplate(1);
    fragmentTC.selectTemplate(1);
    geometryTC.selectTemplate(2);
    applyButton.onclick();

    // Setup render function
    setInterval(render, 40);
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
    return true;
}

function setVertexShader(shaderText) {
    window.console.log("setVertexShader()...");
    vertexShader.attributes = ExtractAttributesFromShaderSource(shaderText);
    vertexShader.uniforms = ExtractUniformsFromShaderSource(shaderText);
    return compileShader(vertexShader, shaderText);
}

function setFragmentShader(shaderText) {
    window.console.log("setFragmentShader()...");
    fragmentShader.uniforms = ExtractUniformsFromShaderSource(shaderText);
    return compileShader(fragmentShader, shaderText);
}

function setShaderData()
{
    for (var attr in attributes) {
        attributes[attr].id = gl.getAttribLocation(shaderProgram, attr);
        if (attributes[attr].id == -1) {
            logInfo("Warning: Attribute '" + attr + "' specified by geometry but not supported in shader. Ignoring attribute array.");
            attributes[attr].buffer = -1;
        }
        else {
            gl.enableVertexAttribArray(attributes[attr].id);

            // Create buffer and upload data
            attributes[attr].buffer = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, attributes[attr].buffer);
            gl.bufferData(gl.ARRAY_BUFFER, new WebGLFloatArray(attributes[attr]), gl.STATIC_DRAW);
        }
    }

    for (var uni in uniforms) {
        var uniform = uniforms[uni];
        uniform.id = gl.getUniformLocation(shaderProgram, uni);
        if (uniform.id == -1) {
            logInfo("Warning: Uniform '" + uni + "' specified by geometry but not supported in shader. Ignoring uniform.");
        }
        else {
            if (typeof(uniform) == 'string') { // Treat as texture
                texture.image = new Image();
                texture.image.onload = function() {
                    logInfo("Texture loaded.");
                    gl.bindTexture(gl.TEXTURE_2D, texture);
                    gl.texImage2D(gl.TEXTURE_2D, 
                                  0,  // level
                                  gl.RGB,  // internalformat (ALPHA, LUMINANCE, 
                                           // LUMINANCE_ALPHA, RGB, RGBA)
                                  gl.RGB,  // format (ALPHA, RGB, RGBA, 
                                           // LUMINANCE, LUMINANCE_ALPHA)
                                  gl.UNSIGNED_BYTE,  // type
                                  texture.image);
                    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
                    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
                    gl.bindTexture(gl.TEXTURE_2D, null);
                }
                texture.image.src = uniform;
            }
        }
    }

    // Verify that all vertex attributes expected by the shader are specified
    for (var i=0;i<vertexShader.attributes.length;i++) {
        if (!(vertexShader.attributes[i] in attributes)) {
            logError("Expected vertex shader attribute missing: " + vertexShader.attributes[i]);
        }
    }

    // Verify that all uniforms expected by the shaders are specified
    for (var i=0;i<vertexShader.uniforms.length;i++) {
        if (!(vertexShader.uniforms[i] in uniforms)) {
            logError("Expected vertex shader uniform missing: " + vertexShader.uniforms[i]);
        }
    } 
    for (var i=0;i<fragmentShader.uniforms.length;i++) {
        if (!(fragmentShader.uniforms[i] in uniforms)) {
            logError("Expected fragment shader uniform missing: " + fragmentShader.uniforms[i]);
        }
    } 

    if (vbo.indices) {
        vbo.buffer = gl.createBuffer();
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, vbo.buffer);
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new WebGLUnsignedShortArray(vbo.indices), gl.STATIC_DRAW);
    }

}

/*
  Generates geometry:
  vertices = flat array of xyz floats
  normals = flat array of xyz floats
  type = triangles, tristrip etc.
  indices = flat array of integers referencing vertex arrays
*/
function setGeometryCode(code) {
    if (vbo.buffer) gl.deleteBuffer(vbo.buffer);
    if (attributes) {
        for (var attr in attributes) {
            if (attributes[attr].id >= 0) gl.disableVertexAttribArray(attributes[attr].id);
            if (attributes[attr].buffer !== -1) gl.deleteBuffer(attributes[attr].buffer);
        };
    }

    attributes = {};
    uniforms = { time: function() { return elapsed; }, modelViewMatrix: function() { return mvMatrix.flatten(); }, projectionMatrix: function() { return pMatrix.flatten(); }};
    vbo = {};
    try {
        eval(code);
    } catch(e) {
        logError(e);
        return false;
    }



    // FIXME: Fill inn default itemsize/numitems here?

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
function mvRotate(ang, v) {
    var arad = ang * Math.PI / 180.0;
    var m = Matrix.Rotation(arad, $V([v[0], v[1], v[2]])).ensure4x4();
    multMatrix(m);
}
var pMatrix;
function perspective(fovy, aspect, znear, zfar) {
    pMatrix = makePerspective(fovy, aspect, znear, zfar);
}

var firstFrameDate;
var elapsed=0;
function render() {
    if (!firstFrameDate) {
        firstFrameDate = new Date();
    } else {
        elapsed = new Date() - firstFrameDate;
    }


    //window.console.log("render()");
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    // FIXME: This uses some utility functions. Evaluate what we want to
    // use of external functionality:
    perspective(45, 1.0, 0.1, 100.0);
    loadIdentity();
    mvTranslate([0.0, 0.0, -7.0]);
    mvRotate(elapsed / 100,[1,1,1]);

    // Render FBO
    for (var attr in attributes) {
        if (attributes[attr].buffer !== -1) {
            gl.bindBuffer(gl.ARRAY_BUFFER, attributes[attr].buffer);
            gl.vertexAttribPointer(attributes[attr].id, attributes[attr].itemsize, gl.FLOAT, false, 0, 0);
        }
    }

    for (var uni in uniforms) {
        if (uniforms[uni].id !== -1) {
            var value = uniforms[uni];
            while (typeof(value) == 'function') value = value();
            if (typeof(value) == 'number')
                gl.uniform1f(uniforms[uni].id, value);
            else if (typeof(value) == 'string') {
                // FIXME: Hardcoded to be a texture using unit 0
                gl.activeTexture(gl.TEXTURE0);
                gl.bindTexture(gl.TEXTURE_2D, texture);
                gl.uniform1i(uniforms[uni].id, 0);
            }
            else
                if (!('length' in value)) 
                    logError('invalid uniform value for "'+uni+'"');
                else {
                    if (value.length <= 4)
                        gl['uniform' + value.length + 'fv'](uniforms[uni].id, value);
                    else if (value.length == 16)
                        gl.uniformMatrix4fv(uniforms[uni].id, false, new WebGLFloatArray(value));
                    else
                        logError('unsupported uniform length ' + value.length + ' for "'+uni+'"');
                }
        }
    }

    if (vbo.indices) {
        gl.drawElements(vbo.mode, vbo.numitems, gl.UNSIGNED_SHORT, 0);
    }
    else {
        gl.drawArrays(vbo.mode, 0, vbo.numitems);
    }
}


function tryRelink() {
    window.console.log("Linking shaders...");
    gl.linkProgram(shaderProgram);
    gl.validateProgram(shaderProgram);
    if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
        logError("error linking program:\n" +
                 gl.getProgramInfoLog(shaderProgram));
        return false;
    }
    
    //FIXME: don't call this every time...
    gl.useProgram(shaderProgram);
    
    // Handle uniforms
    shaderProgram.pMatrixUniform = gl.getUniformLocation(shaderProgram, "uPMatrix");
    shaderProgram.mvMatrixUniform = gl.getUniformLocation(shaderProgram, "uMVMatrix");
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

function clearLog() { 
    logContainer.innerHTML = '';
}



function TemplateControl(textAreaId, selectListId) {
    var textArea = document.getElementById(textAreaId);
    var selectList = document.getElementById(selectListId);
    var me = this;
    var templates = new Object();

    //add custom entry
    selectList.innerHTML = "<option value='custom'>custom</option>";
    selectList.onchange = function() {
        var selectedName = selectList.options[selectList.selectedIndex].value;
        if (selectedName != 'custom') {
            var text = templates[selectedName];
            textArea.value = text;
            if (me.onchange) me.onchange();
        }
    };

    textArea.onkeypress = function() {
        textArea.onchange();
    }

    textArea.onchange = function() {
        selectList.selectedIndex = selectList.options.length-1;
        if (me.onchange)
            me.onchange();
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
    };

    this.getText = function() { return textArea.value; };
}

function loadTextFile(path) {
    var xhr = null;
    xhr = new XMLHttpRequest();

    if (!xhr) return null; 
            

    // Changed from text/xml to text/plan since we're usually 
    // reading javascript or shaders.
    xhr.overrideMimeType("text/plain");

    // Deal with firefox security for file:// urls
    try {
        var nsPM = null;
        if (typeof(netscape) !== 'undefined' && 
            typeof(netscape.security) !== 'undefined' && 
            typeof(netscape.security.PrivilegeManager) !== 'undefined') {
            nsPM = netscape.security.PrivilegeManager;
        }
        if (document.location.href.match(/^file:\/\//)) {
            if (nsPM !== null) {
                nsPM.enablePrivilege("UniversalBrowserRead");
            }
        }
    }catch (e) {
        logError(e);
        throw "\tBrowser security may be restrcting access to local files";
    }

    try {
        xhr.open("GET", path, false);

        // Ignore cache if the file is served from localhost or is at a
        // file:// url as it's safe to assume that's a developer.
        var url = window.location.href;
        if (    url.match(/^http:\/\/localhost/) !== null ||
                url.match(/^http:\/\/127\.0\.0\.1/) !== null ||
                url.match(/^file:/) !== null) {
            xhr.setRequestHeader('Pragma', 'Cache-Control: no-cache');
        }

        xhr.send(null);
    }catch (e) { 
        throw e; 
    }

    //allow status 0 since file loads don't report http status codes
    if (xhr.status !== 200 && xhr.status !== 0) {
        logError("\tCompleted with status: " + xhr.status);
        return "File load error: " + xhr.status;
    }else {
        return xhr.responseText;
    }
}
