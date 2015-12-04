declare var component;

console.log("hello!");

var c = component.list("");

for (var i in c) {
  console.log(i);
}

console.log(component());