let workspace = bazel info workspace | lines | get 0
let test_ecsact_file = $env.FILE_PWD | path join "async_test.ecsact"
let test_recipe_file = $workspace | path join (bazel cquery @ecsact_rt_reference//async_reference --output=files | lines | get 0)
let test_runtime_dll = $env.FILE_PWD | path join "test.dll"

# echo $test_recipe_file

echo (open $test_recipe_file) | get sources

bazel build @ecsact_cli @ecsact_rt_reference//async_reference
bazel run @ecsact_cli -- build $test_ecsact_file -r $test_recipe_file -o $test_runtime_dll
