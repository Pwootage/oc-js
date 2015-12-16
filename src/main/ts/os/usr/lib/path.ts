//Modified from node.js's path module
//found at https://github.com/nodejs/node/blob/master/lib/path.js
//(at time of writing)

export interface ParsedPath {
  root: string;
  dir: string;
  base: string;
  ext: string;
  name: string;
}

// resolves . and .. elements in a path array with directory names there
// must be no slashes or device names (c:\) in the array
// (so also no leading and trailing slashes - it does not distinguish
// relative and absolute paths)
function normalizeArray(parts:string[], allowAboveRoot:boolean) {
  var res = [];
  for (var i = 0; i < parts.length; i++) {
    var p = parts[i];

    // ignore empty parts
    if (!p || p === '.')
      continue;

    if (p === '..') {
      if (res.length && res[res.length - 1] !== '..') {
        res.pop();
      } else if (allowAboveRoot) {
        res.push('..');
      }
    } else {
      res.push(p);
    }
  }

  return res;
}

// Returns an array with empty elements removed from either end of the input
// array or the original array if no elements need to be removed
function trimArray(arr:string[]) {
  var lastIndex = arr.length - 1;
  var start = 0;
  for (; start <= lastIndex; start++) {
    if (arr[start])
      break;
  }

  var end = lastIndex;
  for (; end >= 0; end--) {
    if (arr[end])
      break;
  }

  if (start === 0 && end === lastIndex)
    return arr;
  if (start > end)
    return [];
  return arr.slice(start, end + 1);
}

// Split a filename into [root, dir, basename, ext], unix version
// 'root' is just a slash, or nothing.
const splitPathRe = /^(\/?|)([\s\S]*?)((?:\.{1,2}|[^\/]+?|)(\.[^.\/]*|))(?:[\/]*)$/;

function posixSplitPath(filename:string):RegExpExecArray {
  const out = splitPathRe.exec(filename);
  out.shift();
  return out;
}


export function resolve(...segments:any[]):string {
  var resolvedPath = '',
    resolvedAbsolute = false;

  for (var i = arguments.length - 1; i >= -1 && !resolvedAbsolute; i--) {
    //TODO: CWD?
    //Original: var path = (i >= 0) ? arguments[i] : process.cwd();
    var path = (i >= 0) ? arguments[i] : '/';

    // Skip empty entries
    if (path === '') {
      continue;
    }

    resolvedPath = path + '/' + resolvedPath;
    resolvedAbsolute = path[0] === '/';
  }

  // At this point the path should be resolved to a full absolute path, but
  // handle relative paths to be safe (might happen when process.cwd() fails)

  // Normalize the path
  resolvedPath = normalizeArray(resolvedPath.split('/'),
    !resolvedAbsolute).join('/');

  return ((resolvedAbsolute ? '/' : '') + resolvedPath) || '.';
}

export function normalize(path:string):string {
  var isAbsolute = isAbsolute(path),
    trailingSlash = path && path[path.length - 1] === '/';

  // Normalize the path
  path = normalizeArray(path.split('/'), !isAbsolute).join('/');

  if (!path && !isAbsolute) {
    path = '.';
  }
  if (path && trailingSlash) {
    path += '/';
  }

  return (isAbsolute ? '/' : '') + path;
}

export function isAbsolute(path:string) {
  return !!path && path[0] === '/';
}

export function join(...segments:string[]):string {
  var path = '';
  for (var i = 0; i < segments.length; i++) {
    var segment = segments[i];
    if (segment) {
      if (!path) {
        path += segment;
      } else {
        path += '/' + segment;
      }
    }
  }
  return normalize(path);
}


export function relative(from:string, to:string) {
  from = resolve(from).substr(1);
  to = resolve(to).substr(1);

  var fromParts = trimArray(from.split('/'));
  var toParts = trimArray(to.split('/'));

  var length = Math.min(fromParts.length, toParts.length);
  var samePartsLength = length;
  for (var i = 0; i < length; i++) {
    if (fromParts[i] !== toParts[i]) {
      samePartsLength = i;
      break;
    }
  }

  var outputParts = [];
  for (var i = samePartsLength; i < fromParts.length; i++) {
    outputParts.push('..');
  }

  outputParts = outputParts.concat(toParts.slice(samePartsLength));

  return outputParts.join('/');
}

export function dirname(path:string):string {
  var result = posixSplitPath(path),
    root = result[0],
    dir = result[1];

  if (!root && !dir) {
    // No dirname whatsoever
    return '.';
  }

  if (dir) {
    // It has a dirname, strip trailing slash
    dir = dir.substr(0, dir.length - 1);
  }

  return root + dir;
}


export function basename(path:string, ext?:string) {
  var f = posixSplitPath(path)[2];
  if (ext && f.substr(-1 * ext.length) === ext) {
    f = f.substr(0, f.length - ext.length);
  }
  return f;
}


export function extname(path:string):string {
  return posixSplitPath(path)[3];
}


export function format(pathObject:ParsedPath):string {
  var dir = pathObject.dir || pathObject.root;
  var base = pathObject.base || ((pathObject.name || '') + (pathObject.ext || ''));
  if (!dir) return base;
  if (dir === pathObject.root) return dir + base;
  return dir + sep + base;
}


export function parse(pathString:string):ParsedPath {
  var allParts = posixSplitPath(pathString);
  return {
    root: allParts[0],
    dir: allParts[0] + allParts[1].slice(0, -1),
    base: allParts[2],
    ext: allParts[3],
    name: allParts[2].slice(0, allParts[2].length - allParts[3].length)
  };
}


export const sep = '/';
export const delimiter = ':';