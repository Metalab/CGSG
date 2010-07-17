function initShaders() {
//    $W.GLSL.ShaderProgram('materialtest');
//    firstprog = $W.programs.firstlight;
//    firstprog.attachShader("shader-fs");
//    firstprog.attachShader("shader-vs");
//    firstprog.link();
//    if (error) {
//      alert("Could not initialise shaders");
//    }

//    firstprog.use();
}

function create_sphere() {

    new $W.Material(
        {
            name: "mymaterial",
            program: {
                name: "myshader" ,
                shaders: [ {name:"shader-fs"}, {name:"shader-vs"} ] 
            }
//,
//            textures: [
//                {name:"green_tiger", type:"Image", path:$W.paths.textures + "tiger.png"}
//            ]
        });

    var sphere = new $W.Object($W.GL.TRIANGLES);

    var spheredata = $W.util.genSphere(20, 20, 1);
    sphere.fillArray('vertex', spheredata.vertices);
//    sphere.fillArray('normal', spheredata.normals);
//    sphere.fillArray('color', spheredata.normals);
    sphere.setElements(spheredata.indices);
    sphere.animation.updateRotation = function(dt) { 
        this.setRotation((this.age/20) % 360, 0, 0);
    }

    sphere.setMaterial('mymaterial');
}

function webGLMain() {
    var gl;
    gl = $W.GL;

    initShaders();

    gl.clearColor(0.0, 0.0, 0.0, 1.0);

    $W.camera.setPosition(0.0, 0.0, 2.0);

    create_sphere();

    setInterval(function() {
        $W.update();
        $W.draw();  
    }, 15);
}
