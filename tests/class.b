class Pair {
  var field = 10

  method() {

  }
}

var pair = Pair()
pair.first = 1
pair.second = 2
echo pair.first + pair.second
echo pair.field

class Scone {
  topping(first, second) {
    echo 'scone with ' + first + ' and ' + second
  }
}

var scone = Scone()
scone.topping('berries', 'cream')

class Person {
  var age = 10

  Person(name) {
    self.name = name
  }

  welcome(age) {
    echo 'welcome ' + self.name + '. You are ' + (self.age + age) + ' years old\n'
  }
}

var person1 = Person('Richard')
var person2 = Person('Jane')

person1.welcome(5)
person2.welcome(15)


class A {
  say() {
    echo "A\n"
  }
}

class B < A {
  getClosure() {
    def closure() {
      parent.say()
    }
    return closure
  }

  say() {
    echo "B\n"
  }
}

class C < B {
  say() {
    echo "C\n"
  }
}

C().getClosure()()