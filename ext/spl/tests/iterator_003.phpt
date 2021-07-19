--TEST--
SPL: CachingIterator and __toString()
--FILE--
<?php

class Student
{
    private $id;
    private $name;

    public function __construct($id, $name)
    {
        $this->id = $id;
        $this->name = $name;
    }

    public function __toString()
    {
        return $this->id . ', ' . $this->name;
    }

    public function getId()
    {
        return $this->id;
    }
}

class StudentIdFilter extends FilterIterator
{
    private $id;

    public function __construct(ArrayObject $students, Student $other)
    {
        FilterIterator::__construct($students->getIterator());
        $this->id = $other->getId();
    }

    public function accept(): bool
    {
        echo "ACCEPT ".$this->current()->getId()." == ".$this->id."\n";
        return $this->current()->getId() == $this->id;
    }
}

class StudentList implements IteratorAggregate
{
    private $students;

    public function __construct()
    {
        $this->students = new ArrayObject(array());
    }

    public function add(Student $student)
    {
        if (!$this->contains($student)) {
            $this->students[] = $student;
        }
    }

    public function contains(Student $student)
    {
        foreach ($this->students as $s)
        {
            if ($s->getId() == $student->getId()) {
                return true;
            }
        }
        return false;
    }

    public function getIterator(): Traversable {
        return new CachingIterator($this->students->getIterator(), true);
    }
}

$students = new StudentList();
$students->add(new Student('01234123', 'Joe'));
$students->add(new Student('00000014', 'Bob'));
$students->add(new Student('00000014', 'Foo'));

// The goal is to verify we can access the cached string value even if it was
// generated by a call to __toString(). To check this we need to access the
// iterator's __toString() method.
$it = $students->getIterator();
foreach ($it as $student) {
    echo $it->__toString(), "\n";
}
?>
--EXPECT--
01234123, Joe
00000014, Bob
