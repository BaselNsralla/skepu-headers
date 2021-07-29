// template<typename T, typename It>
// struct ArgIter {
//     using difference_type = void;
//     using value_type = typename It::value_type;
//     using reference = typename It::reference;
//     using pointer = typename It::pointer;
//     using iterator_category = std::input_iterator_tag;


//     T& container;
//     It begin;
//     It end;

//     ArgIter(T& container, It begin, It end): container{container}, begin{begin}, end{end} {}

//     ArgIter& operator++() {
//         ++begin;
//         return *this;
//     }

//     ArgIter operator++(int)
//     {
//         ArgIter tmp{*this};
//         ++(*this);
//         return tmp;
//     }
  
//     bool operator==(ArgIter const& other) const
//     {
//         return begin == other.begin && end == other.end;
//     }

//     bool operator!=(ArgIter const& other) const
//     {
//         return !(*this == other);
//     }

//     value_type operator*()
//     {
//         return *begin;
//     }

//     pointer operator->()
//     {
//         return &(*begin);
//     }
// };


struct ArgPerm // zip även här // Inner 
{
    std::vector<std::vector<Size>> input;
    std::vector<ArgCombo> permutation_sequence;

    void add(std::vector<Size> possibilites) 
    { // && och std::move
        input.push_back(possibilites);
    }

    void permutations() 
    {
        if (input.empty()) { return; }
        
        if (input.size() > 1) 
        {
            auto first = input[0];
            for (auto& startItem: first) // auto = Size 
            {
                apply({startItem}, std::next(input.begin()));
            }

        } else {
            permutation_sequence = std::vector<ArgCombo>{{std::move(input)}};
        }
    }

    //ArgIter

    decltype(permutation_sequence.begin()) 
    begin() { return permutation_sequence.begin();}

    decltype(permutation_sequence.end())
    end() { return permutation_sequence.end(); }

    size_t size() { return permutation_sequence.size(); }

private:
    template<typename T>
    void apply(ArgCombo&& combo, T restIt) 
    {    
        if (restIt == input.end()) 
        {
            permutation_sequence.push_back(combo);
        }

        for (auto& size: *restIt) 
        {   
            combo.push_back(size);
            apply(std::move(combo), std::next(restIt));
        }
    }
};


struct ArgCatPerm 
{
    std::vector<ArgPerm> input;
    std::vector<ArgCatCombo> permutation_sequence;
    //Mechanism constructor

    void add(ArgPerm&& possibilites) 
    { // && och std::move
        input.push_back(std::move(possibilites));
    }
    
    void permutations(Mechanism mechanism = Mechanism::ALL) 
    {
        if (input.empty()) { return; }
        
        auto first = input[0];
        //for (auto& startItem: first) // Impolementera Iterator för ArgPerm
        for(auto it = first.begin(); size_t i = 0; it != first.end(); ++it; ++i)
        {
            auto& startItem = *it;
            switch (mechanism)
            {
            case Mechanism::ALL:
                apply({startItem}, std::next(input.begin()));
                break;

            case Mechanism::SYMETRIC:
                bool zipped = zip({startItem}, std::next(input.begin()), i);
                if (!zipped) { return; }
                break;
                
            default:
                break;
            }
        }
    }



    // ArgCatPerm operator+(ArgCatPerm& other) {

    // }

    decltype(permutation_sequence.begin()) 
    begin() { return permutation_sequence.begin();}

    decltype(permutation_sequence.end())
    end() { return permutation_sequence.end(); }

private:
    template<typename T>
    void apply(ArgCatCombo&& combo, T restIt) 
    {    
        if (restIt == input.end()) 
        {
            permutation_sequence.push_back(combo);
        }

        for (auto& size: *restIt) 
        {   
            combo.push_back(size);
            apply(std::move(combo), std::next(restIt));
        }
    }

    template<typename T>
    bool zip(ArgCatCombo&& combo, T restIt, size_t i) 
    {
        if (restIt == input.end()) 
        {
            permutation_sequence.push_back(combo);
        }

        for (auto it = restIt; it != input.end(); ++it)
        {
            if (i >= (*it).size())
            {
                return false;
            } else 
            {
                combo.push_back((*it).permutation_sequence[i]);
            }
        }

        permutation_sequence.push_back(combo);
        return true;
    }

};

