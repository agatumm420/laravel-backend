<?php

namespace App\Http\Livewire;

use Livewire\Component;

class Frontpage extends Component
{   public $title;
    public $content;
    public function redirLogin(){
        return redirect(route('login'));
    }
    public function  redirRegister(){
        return redirect(route('register'));
    }
    public function render()
    {
        return view('livewire.frontpage');
    }
}
