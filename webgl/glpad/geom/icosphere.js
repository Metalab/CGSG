
//generate icosahedron
var t = (1.0 + Math.sqrt(5.0))/2.0;

function normalizeLength(arr) {
    for(var i=0; i < arr.length; i+=3) {
        len = Math.sqrt(arr[i]*arr[i] + arr[i+1]*arr[i+1] + arr[i+2]*arr[i+2]);
        arr[i] /= len;
        arr[i+1] /= len;
        arr[i+2] /= len;
    }
};

attributes.vertex = [
-1,  t,  0,
 1,  t,  0,
-1, -t,  0,
 1, -t,  0,

 0, -1,  t,
 0,  1,  t,
 0, -1, -t,
 0,  1, -t,

 t,  0, -1,
 t,  0,  1,
-t,  0, -1,
-t,  0,  1
];

normalizeLength(attributes.vertex);

attributes.vertex.itemsize = 3;

vbo.mode = gl.TRIANGLES;

vbo.indices = [
// 5 faces around point 0
0, 11, 5,
0, 5, 1,
0, 1, 7,
0, 7, 10,
0, 10, 11,

// 5 adjacent faces
1, 5, 9,
5, 11, 4,
11, 10, 2,
10, 7, 6,
7, 1, 8,

// 5 faces around point 3
3, 9, 4,
3, 4, 2,
3, 2, 6,
3, 6, 8,
3, 8, 9,

// 5 adjacent faces
4, 9, 5,
2, 4, 11,
6, 2, 10,
8, 6, 7,
9, 8, 1
];

function getMiddlePoint(cache, idxA, idxB) {
    var vertices = attributes.vertex;

    //try to find the point in the cache
    if (idxA > idxB) 
    { 
        var tmp = idxA; 
        idxA = idxB; 
        idxB = tmp; 
    }
    var key = idxA + '-' + idxB;
    if (key in cache) return cache[key];
    idxA *= 3;
    idxB *= 3;
    middle = [(vertices[idxA] + vertices[idxB])/2.0, 
           (vertices[idxA+1] + vertices[idxB+1])/2.0,
           (vertices[idxA+2] + vertices[idxB+2])/2.0];
    normalizeLength(middle);
    var middleIdx = vertices.length/3;
    vertices.push(middle[0]); 
    vertices.push(middle[1]); 
    vertices.push(middle[2]);
    cache[key] = middleIdx;
    return middleIdx;
}

function refine() {
    var ni = [];
    var cache = new Object();
    for (var i=0; i < vbo.indices.length; i+=3) {
        var a = getMiddlePoint(cache, vbo.indices[i], vbo.indices[i+1]);
        var b = getMiddlePoint(cache, vbo.indices[i+1], vbo.indices[i+2]);
        var c = getMiddlePoint(cache, vbo.indices[i+2], vbo.indices[i]);
        
        ni.push(vbo.indices[i]); ni.push(a); ni.push(c);
        ni.push(vbo.indices[i+1]); ni.push(b); ni.push(a);
        ni.push(vbo.indices[i+2]); ni.push(c); ni.push(b);
        ni.push(a); ni.push(b); ni.push(c);
    }

    vbo.indices = ni;
}

for (var i=0; i < 3; i++)
    refine();

vbo.indices.itemsize=1;
vbo.numitems=vbo.indices.length;
