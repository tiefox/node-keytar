var binary = require('node-pre-gyp');
var path = require('path');
var PACKAGEJSON = path.join(__dirname, 'package.json');
var debug = require('debug')('keytar:binding');

var runtime = 'node';
if (process.versions.electron) {
  runtime = 'electron';
} else if (process.versions['node-webkit']) {
  runtime = 'node-webkit';
}

var opts = {
  runtime: runtime
};

debug('looking up pre-gyp path using %s with options',
  PACKAGEJSON, opts);

var src = binary.find(PACKAGEJSON, opts);
debug('pre-gyp path resolved to %s', src);

try {
  module.exports = require(src);
  debug('successfully required pre-gyp artifact');
} catch (err) {
  debug('failed to required pre-gyp artifact', err);
  debug('failling back to node-gyp artifact');
  module.exports = require('./build/Release/keytar.node');
}
