function ExtractUniformsFromShaderSource(source) {
    var reg = 
        new RegExp(
            "uniform ((bool|int|uint|float|[biu]?vec[234]|mat[234]x?[234]?|sampler2D) ([A-Za-z0-9]*));",
            "gi");
    var tmp;
    var uniforms = [];
    while (tmp = reg.exec(source)) {
	uniforms.push(tmp[3]);
    }
    return uniforms;
}

function ExtractAttributesFromShaderSource(source) {
    var reg = new RegExp("attribute ((bool|int|uint|float|[biu]?vec[234]|mat[234]x?[234]?) ([A-Za-z0-9]*));", "gi");
    var tmp;
    var attributes = [];
    while (tmp = reg.exec(source)) {
	attributes.push(tmp[3]);
    }
    return attributes;
}
