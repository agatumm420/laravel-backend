@if(Auth::check())
<nav x-data="{ open: false }" class="bg-white border-b border-gray-100">
    <!-- Primary Navigation Menu -->
    <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div class="flex justify-between h-16">
            <div class="flex">
                <!-- Logo -->
                
                    <svg version="1.1" id="keune-logo-2" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px" width="162px" height="59px" viewBox="0 0 162 59" enable-background="new 0 0 162 59" xml:space="preserve">
                        <g>
                            <path fill="#222323" d="M20.031,47.826L6.775,34.606l20.832-20.647l-2.258-0.034L3.444,35.092l-0.07-0.555l0.01-20.578H1.483v44.06
                                h1.961V37.937l1.943-1.735l22.407,21.804l2.637,0.014L20.031,47.826z M31.468,58.019h20.039l-0.008-1.854H33.458l0.001-21.021
                                l18.04,0.012v-2.143l-18.04-0.002l-0.033-17.004l18.051-0.01v-1.898l-20.032,0.005L31.468,58.019z M83.589,41.808
                                c-0.138-0.277-0.059-27.849-0.059-27.849l-1.839-0.006l-0.06,31.825c-0.172,1.873-0.771,3.332-1.466,4.791
                                c-1.701,3.364-5.727,5.931-10.204,6.279c-6.07,0.832-11.415-2.532-13.634-6.557c-0.801-1.217-1.047-2.813-1.325-4.374
                                l-0.006-31.854c-0.589,0-1.794,0.016-1.794,0.016s0,20.892,0.001,31.502c0.042,2.747,1.022,4.803,1.597,5.716
                                c2.429,4.579,8.465,8.187,15.162,7.354c5.275-0.451,9.889-3.435,11.868-7.321C83.323,48.8,83.624,45.416,83.589,41.808
                                 M124.963,58.235l0.009-44.275h-2.022v39.314c-1.005-0.832-1.769-1.839-2.638-2.777l-26.89-28.001l-8.226-8.397l0.031,43.933
                                l2.077-0.004V18.991h0.112l16.032,16.725L115.975,48.9l8.743,9.335H124.963z M137.35,13.96h-10.826v44.059l19.848,0.01V56.05
                                l-17.84,0.035V35.127l17.84,0.017v-2.166l-17.84-0.002V15.751h8.828L137.35,13.96z M160.521,9.589
                                c-0.072,0.376-0.289,0.701-0.607,0.917c-0.238,0.159-0.516,0.244-0.801,0.244c-0.48,0-0.929-0.236-1.196-0.632
                                c-0.442-0.658-0.268-1.552,0.39-1.996c0.238-0.162,0.516-0.245,0.801-0.245c0.479,0,0.927,0.235,1.193,0.634
                                C160.516,8.828,160.593,9.212,160.521,9.589 M153.388,17.419c0,3.471-2.823,6.293-6.294,6.293c-3.47,0-6.293-2.823-6.293-6.293
                                c0-3.471,2.823-6.295,6.293-6.295C150.564,11.124,153.388,13.948,153.388,17.419 M147.094,4.362c-0.793,0-1.437-0.645-1.437-1.438
                                c0-0.792,0.644-1.437,1.437-1.437c0.794,0,1.438,0.645,1.438,1.437C148.531,3.718,147.888,4.362,147.094,4.362 M152.63,3.946
                                c0.25-0.469,0.736-0.763,1.272-0.763c0.234,0,0.461,0.058,0.671,0.169c0.339,0.181,0.587,0.481,0.7,0.85
                                c0.112,0.366,0.075,0.754-0.105,1.094c-0.252,0.47-0.736,0.761-1.271,0.761c-0.235,0-0.464-0.055-0.672-0.169
                                c-0.341-0.179-0.587-0.48-0.702-0.848C152.41,4.676,152.449,4.286,152.63,3.946 M141.663,5.042c-0.11,0.368-0.36,0.669-0.698,0.848
                                c-0.211,0.114-0.436,0.169-0.673,0.169c-0.533,0-1.021-0.293-1.271-0.762c-0.18-0.339-0.217-0.728-0.104-1.093
                                c0.11-0.369,0.359-0.669,0.698-0.85c0.21-0.112,0.438-0.169,0.673-0.169c0.536,0,1.021,0.293,1.271,0.763
                                C141.739,4.285,141.776,4.676,141.663,5.042 M136.271,10.118c-0.27,0.396-0.716,0.632-1.194,0.632
                                c-0.287,0-0.564-0.085-0.803-0.244c-0.318-0.216-0.534-0.541-0.607-0.917c-0.072-0.377,0.006-0.761,0.221-1.078
                                c0.266-0.398,0.712-0.634,1.193-0.634c0.287,0,0.563,0.083,0.801,0.245C136.539,8.566,136.714,9.46,136.271,10.118 M161.714,9.82
                                c0.134-0.696-0.01-1.402-0.403-1.989c-0.496-0.732-1.318-1.169-2.203-1.169c-0.527,0-1.039,0.157-1.479,0.454
                                c-1.034,0.698-1.371,1.989-0.954,3.109l-3.459,2.332c-0.543-0.683-1.179-1.28-1.916-1.751l1.961-3.69
                                c0.214,0.055,0.415,0.159,0.637,0.159h0.002c0.982,0,1.881-0.539,2.343-1.407c0.331-0.625,0.401-1.343,0.193-2.021
                                c-0.207-0.677-0.665-1.234-1.289-1.566c-0.383-0.204-0.815-0.311-1.243-0.311c-0.985,0-1.886,0.539-2.346,1.407
                                c-0.331,0.625-0.401,1.344-0.195,2.02c0.147,0.475,0.45,0.855,0.811,1.176l-1.941,3.653c-0.781-0.343-1.637-0.531-2.529-0.601
                                v-4.17c1.162-0.282,2.044-1.282,2.044-2.529c0-1.463-1.188-2.65-2.651-2.65c-1.46,0-2.652,1.188-2.652,2.65
                                c0,1.247,0.884,2.248,2.046,2.529v4.17c-0.891,0.069-1.748,0.258-2.531,0.603l-1.941-3.655c0.364-0.32,0.667-0.701,0.811-1.176
                                c0.21-0.676,0.138-1.395-0.193-2.02c-0.462-0.868-1.36-1.407-2.344-1.407c-0.431,0-0.86,0.107-1.243,0.311
                                c-0.624,0.332-1.084,0.889-1.289,1.566c-0.21,0.678-0.14,1.396,0.193,2.021c0.462,0.868,1.357,1.407,2.343,1.407
                                c0.22,0,0.421-0.104,0.637-0.159l1.963,3.69c-0.737,0.471-1.375,1.068-1.919,1.751l-3.458-2.332
                                c0.417-1.121,0.082-2.411-0.952-3.109c-0.44-0.297-0.952-0.454-1.481-0.454c-0.885,0-1.707,0.437-2.197,1.169
                                c-0.4,0.587-0.544,1.293-0.407,1.988c0.136,0.695,0.533,1.297,1.12,1.693c0.438,0.297,0.951,0.454,1.48,0.454
                                c0.667,0,1.253-0.312,1.735-0.75l3.479,2.346c-0.651,1.144-1.053,2.448-1.053,3.856c0,4.332,3.524,7.854,7.854,7.854
                                c4.333,0,7.855-3.522,7.855-7.854c0-1.41-0.401-2.712-1.053-3.856l3.479-2.346c0.484,0.438,1.066,0.75,1.737,0.75
                                c0.527,0,1.04-0.157,1.479-0.454C161.179,11.116,161.578,10.515,161.714,9.82"></path>
                        </g>
                        </svg>
                

                <!-- Navigation Links -->
                <div class="hidden space-x-8 sm:-my-px sm:ml-10 sm:flex">
                    <x-jet-nav-link href="{{ route('dashboard') }}" :active="request()->routeIs('dashboard')">
                        {{ __('Dashboard') }}
                    </x-jet-nav-link>
                    
                    <x-jet-nav-link href="{{ route('messages') }}" :active="request()->routeIs('messages')">
                        {{ __('Messages') }}
                    </x-jet-nav-link>
                </div>
            </div>

            <div class="hidden sm:flex sm:items-center sm:ml-6">
                <!-- Teams Dropdown -->
                @if (Laravel\Jetstream\Jetstream::hasTeamFeatures())
                    <div class="ml-3 relative">
                        <x-jet-dropdown align="right" width="60">
                            <x-slot name="trigger">
                                <span class="inline-flex rounded-md">
                                    <button type="button" class="inline-flex items-center px-3 py-2 border border-transparent text-sm leading-4 font-medium rounded-md text-gray-500 bg-white hover:bg-gray-50 hover:text-gray-700 focus:outline-none focus:bg-gray-50 active:bg-gray-50 transition">
                                        {{ Auth::user()->currentTeam->name }}

                                        <svg class="ml-2 -mr-0.5 h-4 w-4" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor">
                                            <path fill-rule="evenodd" d="M10 3a1 1 0 01.707.293l3 3a1 1 0 01-1.414 1.414L10 5.414 7.707 7.707a1 1 0 01-1.414-1.414l3-3A1 1 0 0110 3zm-3.707 9.293a1 1 0 011.414 0L10 14.586l2.293-2.293a1 1 0 011.414 1.414l-3 3a1 1 0 01-1.414 0l-3-3a1 1 0 010-1.414z" clip-rule="evenodd" />
                                        </svg>
                                    </button>
                                </span>
                            </x-slot>

                            <x-slot name="content">
                                <div class="w-60">
                                    <!-- Team Management -->
                                    <div class="block px-4 py-2 text-xs text-gray-400">
                                        {{ __('Manage Team') }}
                                    </div>

                                    <!-- Team Settings -->
                                    <x-jet-dropdown-link href="{{ route('teams.show', Auth::user()->currentTeam->id) }}">
                                        {{ __('Team Settings') }}
                                    </x-jet-dropdown-link>

                                    @can('create', Laravel\Jetstream\Jetstream::newTeamModel())
                                        <x-jet-dropdown-link href="{{ route('teams.create') }}">
                                            {{ __('Create New Team') }}
                                        </x-jet-dropdown-link>
                                    @endcan

                                    <div class="border-t border-gray-100"></div>

                                    <!-- Team Switcher -->
                                    <div class="block px-4 py-2 text-xs text-gray-400">
                                        {{ __('Switch Teams') }}
                                    </div>

                                    @foreach (Auth::user()->allTeams() as $team)
                                        <x-jet-switchable-team :team="$team" />
                                    @endforeach
                                </div>
                            </x-slot>
                        </x-jet-dropdown>
                    </div>
                @endif

                <!-- Settings Dropdown -->
                <div class="ml-3 relative">
                    <x-jet-dropdown align="right" width="48">
                        <x-slot name="trigger">
                            @if (Laravel\Jetstream\Jetstream::managesProfilePhotos())
                                <button class="flex text-sm border-2 border-transparent rounded-full focus:outline-none focus:border-gray-300 transition">
                                    <img class="h-8 w-8 rounded-full object-cover" src="{{ Auth::user()->profile_photo_url }}" alt="{{ Auth::user()->name }}" />
                                </button>
                            @else
                                <span class="inline-flex rounded-md">
                                    <button type="button" class="inline-flex items-center px-3 py-2 border border-transparent text-sm leading-4 font-medium rounded-md text-gray-500 bg-white hover:text-gray-700 focus:outline-none transition">
                                        {{ Auth::user()->name }}

                                        <svg class="ml-2 -mr-0.5 h-4 w-4" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor">
                                            <path fill-rule="evenodd" d="M5.293 7.293a1 1 0 011.414 0L10 10.586l3.293-3.293a1 1 0 111.414 1.414l-4 4a1 1 0 01-1.414 0l-4-4a1 1 0 010-1.414z" clip-rule="evenodd" />
                                        </svg>
                                    </button>
                                </span>
                            @endif
                        </x-slot>

                        <x-slot name="content">
                            <!-- Account Management -->
                            <div class="block px-4 py-2 text-xs text-gray-400">
                                {{ __('Manage Account') }}
                            </div>

                            <x-jet-dropdown-link href="{{ route('profile.show') }}">
                                {{ __('Profile') }}
                            </x-jet-dropdown-link>

                            @if (Laravel\Jetstream\Jetstream::hasApiFeatures())
                                <x-jet-dropdown-link href="{{ route('api-tokens.index') }}">
                                    {{ __('API Tokens') }}
                                </x-jet-dropdown-link>
                            @endif

                            <div class="border-t border-gray-100"></div>

                            <!-- Authentication -->
                            <form method="POST" action="{{ route('logout') }}" x-data>
                                @csrf

                                <x-jet-dropdown-link href="{{ route('logout') }}"
                                         @click.prevent="$root.submit();">
                                    {{ __('Log Out') }}
                                </x-jet-dropdown-link>
                            </form>
                        </x-slot>
                    </x-jet-dropdown>
                </div>
            </div>

            <!-- Hamburger -->
            <div class="-mr-2 flex items-center sm:hidden">
                <button @click="open = ! open" class="inline-flex items-center justify-center p-2 rounded-md text-gray-400 hover:text-gray-500 hover:bg-gray-100 focus:outline-none focus:bg-gray-100 focus:text-gray-500 transition">
                    <svg class="h-6 w-6" stroke="currentColor" fill="none" viewBox="0 0 24 24">
                        <path :class="{'hidden': open, 'inline-flex': ! open }" class="inline-flex" stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 6h16M4 12h16M4 18h16" />
                        <path :class="{'hidden': ! open, 'inline-flex': open }" class="hidden" stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
                    </svg>
                </button>
            </div>
        </div>
    </div>

    <!-- Responsive Navigation Menu -->
    <div :class="{'block': open, 'hidden': ! open}" class="hidden sm:hidden">
        <div class="pt-2 pb-3 space-y-1">
            <x-jet-responsive-nav-link href="{{ route('dashboard') }}" :active="request()->routeIs('dashboard')">
                {{ __('Dashboard') }}
            </x-jet-responsive-nav-link>
        </div>

        <!-- Responsive Settings Options -->
        <div class="pt-4 pb-1 border-t border-gray-200">
            <div class="flex items-center px-4">
                @if (Laravel\Jetstream\Jetstream::managesProfilePhotos())
                    <div class="shrink-0 mr-3">
                        <img class="h-10 w-10 rounded-full object-cover" src="{{ Auth::user()->profile_photo_url }}" alt="{{ Auth::user()->name }}" />
                    </div>
                @endif

                <div>
                    <div class="font-medium text-base text-gray-800">{{ Auth::user()->name }}</div>
                    <div class="font-medium text-sm text-gray-500">{{ Auth::user()->email }}</div>
                </div>
            </div>

            <div class="mt-3 space-y-1">
                <!-- Account Management -->
                <x-jet-responsive-nav-link href="{{ route('profile.show') }}" :active="request()->routeIs('profile.show')">
                    {{ __('Profile') }}
                </x-jet-responsive-nav-link>

                @if (Laravel\Jetstream\Jetstream::hasApiFeatures())
                    <x-jet-responsive-nav-link href="{{ route('api-tokens.index') }}" :active="request()->routeIs('api-tokens.index')">
                        {{ __('API Tokens') }}
                    </x-jet-responsive-nav-link>
                @endif

                <!-- Authentication -->
                <form method="POST" action="{{ route('logout') }}" x-data>
                    @csrf

                    <x-jet-responsive-nav-link href="{{ route('logout') }}"
                                   @click.prevent="$root.submit();">
                        {{ __('Log Out') }}
                    </x-jet-responsive-nav-link>
                </form>

                <!-- Team Management -->
                @if (Laravel\Jetstream\Jetstream::hasTeamFeatures())
                    <div class="border-t border-gray-200"></div>

                    <div class="block px-4 py-2 text-xs text-gray-400">
                        {{ __('Manage Team') }}
                    </div>

                    <!-- Team Settings -->
                    <x-jet-responsive-nav-link href="{{ route('teams.show', Auth::user()->currentTeam->id) }}" :active="request()->routeIs('teams.show')">
                        {{ __('Team Settings') }}
                    </x-jet-responsive-nav-link>

                    @can('create', Laravel\Jetstream\Jetstream::newTeamModel())
                        <x-jet-responsive-nav-link href="{{ route('teams.create') }}" :active="request()->routeIs('teams.create')">
                            {{ __('Create New Team') }}
                        </x-jet-responsive-nav-link>
                    @endcan

                    <div class="border-t border-gray-200"></div>

                    <!-- Team Switcher -->
                    <div class="block px-4 py-2 text-xs text-gray-400">
                        {{ __('Switch Teams') }}
                    </div>

                    @foreach (Auth::user()->allTeams() as $team)
                        <x-jet-switchable-team :team="$team" component="jet-responsive-nav-link" />
                    @endforeach
                @endif
            </div>
        </div>
    </div>
</nav>
@endif
