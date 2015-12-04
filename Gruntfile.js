module.exports = function(grunt) {

  // Project configuration.
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json'),
    ts: {
      main: {
        options: {
          module: "commonjs",
          target: "es5",
          rootDir: "src/main/ts",
          sourceMap: false
        },
        src: ["src/main/ts/**/*.ts"],
        outDir: "build/ts"
      }
    },
    uglify: {
      build: {
        files: [{
          expand: true,
          cwd: 'build/ts',
          src: '**/*.js',
          dest: 'src/main/resources/assets/oc/js/'
        }]
      }
    }
  });

  // Load the plugin that provides the "uglify" task.
  grunt.loadNpmTasks('grunt-contrib-uglify');
  grunt.loadNpmTasks('grunt-ts');
  grunt.loadNpmTasks('grunt-newer');

  // Default task(s).
  grunt.registerTask('default', ['ts', 'newer:uglify']);

};