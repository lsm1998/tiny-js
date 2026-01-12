// 测试对象字面量
let obj = {
    name: 'lsm',
    age: 30,
    active: true
};

println('obj:', obj);
println('name:', obj.name);
println('age:', obj.age);
println('active:', obj.active);

// 嵌套对象
let user = {
    name: 'Alice',
    profile: {
        age: 25,
        city: 'Beijing'
    }
};

println('user:', user);
println('profile:', user.profile);

// 空对象
let empty = {};
println('empty:', empty);

// 对象作为函数参数
function printPerson(person) {
    println('Person:', person.name, 'Age:', person.age);
}

printPerson({ name: 'Bob', age: 35 });


println('typeof is ', typeof (empty));