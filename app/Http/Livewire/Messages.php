<?php

namespace App\Http\Livewire;
use App\Models\Post;
use Illuminate\Support\Str;
use Illuminate\Validation\Rule;
use Livewire\Component;
use Livewire\WithPagination;

class Messages extends Component
{
    use WithPagination;
    public $PostId;
    public $modalConfirmDeleteVisible = false;
    public $name;
    public $email;
    public $province;
    public $question;
    public function rules()
    {
        return [
            'name' => 'required',
            'email' => 'required',
            'province' => 'required',
            
        ];
    }
    public function mount()
    {
        // Resets the pagination after reloading the page
        $this->resetPage();
    }
     /**
     * The read function.
     *
     * @return void
     */

    public function read()
    {
        return Post::paginate(5);
    }
         /**
     * The delete function.
     *
     * @return void
     */
    public function update()
    {
        $this->validate();
       
        Page::find($this->PostId)->update($this->modelData());
        

        $this->dispatchBrowserEvent('event-notification', [
            'eventName' => 'Updated Page',
            'eventMessage' => 'There is a page (' . $this->PostId . ') that has been updated!',
        ]);
    }
    public function delete()
    {
        Post::destroy($this->PostId);
        $this->modalConfirmDeleteVisible = false;
        $this->resetPage();

        $this->dispatchBrowserEvent('event-notification', [
            'eventName' => 'Deleted',
            'eventMessage' => 'The post (' . $this->PostId . ') has been deleted!',
        ]);
    }   
    public function loadModel()
    {
        $data = Post::find($this->PostId);
        $this->name = $data->title;
        $this->email = $data->email;
        $this->province = $data->province;
        $this->question = $data->question;
        
    }
    public function modelData()
    {
        return [
            'name' => $this->name,
            'email' => $this->email,
            'province' => $this->province,
            'question'=>$this->question,
        ];
    }
    public function dispatchEvent()
    {
        $this->dispatchBrowserEvent('event-notification', [
            'eventName' => 'Sample Event',
            'eventMessage' => 'You have a sample event notification!',
        ]);
    }
    public function deleteShowModal($id)
    {
        $this->PostId = $id;
        $this->modalConfirmDeleteVisible = true;
    }
    public function render()
    {
        return view('livewire.messages',['data' => $this->read(),]);
    }
}
