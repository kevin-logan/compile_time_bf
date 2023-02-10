#include <fmt/format.h>

#define TEST_STATIC_STRING
#include "type_helpers.hpp"

template<static_string_of_concept<int8_t> Data = basic_static_string<int8_t, 0, 0, 0, 0>>
struct bf_memory
{
    static constexpr std::size_t size = Data::size;

    template<std::size_t I>
    struct get_value_helper : std::integral_constant<int8_t, 0>
    {
    };

    template<std::size_t I>
        requires(I < Data::size)
    struct get_value_helper<I> : std::integral_constant<int8_t, Data::template at<I>>
    {
    };

    template<std::size_t I>
    static constexpr auto get_value = get_value_helper<I>::value;

    template<std::size_t I, int8_t V>
    struct set_value_helper
    {
        static constexpr std::size_t size_needed   = I + 1;
        static constexpr std::size_t growth_needed = size_needed - Data::size - 1; // minus 1 to append the new value
        using type =
            bf_memory<typename Data::template append<make_static_string<int8_t, growth_needed>>::template append_chars<V>>;
    };

    template<std::size_t I, int8_t V>
        requires(I < Data::size)
    struct set_value_helper<I, V>
    {
        using type = bf_memory<typename Data::template replace<I, V>>;
    };

    template<std::size_t I, int8_t V>
    using set_value = set_value_helper<I, V>::type;
};

template<static_string_of_concept<char> Input, std::size_t I>
struct bf_get_instruction : std::integral_constant<char, 0>
{
};

template<static_string_of_concept<char> Code, std::size_t I>
    requires(I < Code::size)
struct bf_get_instruction<Code, I> : std::integral_constant<char, Code::template at<I>>
{
};

template<
    static_string_of_concept<char> Code,
    static_string_of_concept<char> Input                    = basic_static_string<char>,
    typename Memory                                         = bf_memory<>,
    std::size_t                           InterpretPosition = 0,
    std::size_t                           Ptr               = 0,
    static_string_of_concept<char>        Output            = basic_static_string<char>,
    static_string_of_concept<std::size_t> LoopStack         = basic_static_string<std::size_t>,
    char                                  Instruction       = bf_get_instruction<Code, InterpretPosition>::value>
struct bf_interpreter
{
};

template<
    static_string_of_concept<char> Code,
    static_string_of_concept<char> Input,
    typename Memory,
    std::size_t                           InterpretPosition,
    std::size_t                           Ptr,
    static_string_of_concept<char>        Output,
    static_string_of_concept<std::size_t> LoopStack>
struct bf_interpreter<Code, Input, Memory, InterpretPosition, Ptr, Output, LoopStack, '\0'>
{
    struct final_result
    {
        static constexpr std::size_t memory_usage = Memory::size;
        using output                              = Output;
    };

    using result = final_result;
};

template<
    static_string_of_concept<char> Code,
    static_string_of_concept<char> Input,
    typename Memory,
    std::size_t                           InterpretPosition,
    std::size_t                           Ptr,
    static_string_of_concept<char>        Output,
    static_string_of_concept<std::size_t> LoopStack>
struct bf_interpreter<Code, Input, Memory, InterpretPosition, Ptr, Output, LoopStack, '+'>
{
    using result = bf_interpreter<
        Code,
        Input,
        typename Memory::template set_value<Ptr, static_cast<int8_t>(Memory::template get_value<Ptr> + 1)>,
        InterpretPosition + 1,
        Ptr,
        Output,
        LoopStack>::result;
};

template<
    static_string_of_concept<char> Code,
    static_string_of_concept<char> Input,
    typename Memory,
    std::size_t                           InterpretPosition,
    std::size_t                           Ptr,
    static_string_of_concept<char>        Output,
    static_string_of_concept<std::size_t> LoopStack>
struct bf_interpreter<Code, Input, Memory, InterpretPosition, Ptr, Output, LoopStack, '-'>
{
    using result = bf_interpreter<
        Code,
        Input,
        typename Memory::template set_value<Ptr, static_cast<int8_t>(Memory::template get_value<Ptr> - 1)>,
        InterpretPosition + 1,
        Ptr,
        Output,
        LoopStack>::result;
};

template<
    static_string_of_concept<char> Code,
    static_string_of_concept<char> Input,
    typename Memory,
    std::size_t                           InterpretPosition,
    std::size_t                           Ptr,
    static_string_of_concept<char>        Output,
    static_string_of_concept<std::size_t> LoopStack>
struct bf_interpreter<Code, Input, Memory, InterpretPosition, Ptr, Output, LoopStack, '>'>
{
    using result = bf_interpreter<Code, Input, Memory, InterpretPosition + 1, Ptr + 1, Output, LoopStack>::result;
};

template<
    static_string_of_concept<char> Code,
    static_string_of_concept<char> Input,
    typename Memory,
    std::size_t                           InterpretPosition,
    std::size_t                           Ptr,
    static_string_of_concept<char>        Output,
    static_string_of_concept<std::size_t> LoopStack>
struct bf_interpreter<Code, Input, Memory, InterpretPosition, Ptr, Output, LoopStack, '<'>
{
    using result = bf_interpreter<Code, Input, Memory, InterpretPosition + 1, Ptr - 1, Output, LoopStack>::result;
};

template<
    static_string_of_concept<char> Code,
    static_string_of_concept<char> Input,
    typename Memory,
    std::size_t                           InterpretPosition,
    std::size_t                           Ptr,
    static_string_of_concept<char>        Output,
    static_string_of_concept<std::size_t> LoopStack>
struct bf_interpreter<Code, Input, Memory, InterpretPosition, Ptr, Output, LoopStack, '.'>
{
    using result =
        bf_interpreter<Code, Input, Memory, InterpretPosition + 1, Ptr, typename Output::push_back<Memory::template get_value<Ptr>>, LoopStack>::
            result;
};

template<
    static_string_of_concept<char> Code,
    static_string_of_concept<char> Input,
    typename Memory,
    std::size_t                           InterpretPosition,
    std::size_t                           Ptr,
    static_string_of_concept<char>        Output,
    static_string_of_concept<std::size_t> LoopStack>
    requires(Input::size > 0)
struct bf_interpreter<Code, Input, Memory, InterpretPosition, Ptr, Output, LoopStack, ','>
{
    using result = bf_interpreter<
        Code,
        typename Input::pop_front<>,
        typename Memory::template set_value<Ptr, static_cast<int8_t>(Input::front)>,
        InterpretPosition + 1,
        Ptr,
        Output,
        LoopStack>::result;
};

template<
    static_string_of_concept<char> Code,
    static_string_of_concept<char> Input,
    typename Memory,
    std::size_t                           InterpretPosition,
    std::size_t                           Ptr,
    static_string_of_concept<char>        Output,
    static_string_of_concept<std::size_t> LoopStack>
    requires(Input::size == 0)
struct bf_interpreter<Code, Input, Memory, InterpretPosition, Ptr, Output, LoopStack, ','>
{
    static_assert(Input::size > 0, "Brainfuck Error: tried to read more from input than was provided");
};

template<
    static_string_of_concept<char> Code,
    std::size_t                    CheckPosition,
    std::size_t                    StackSize,
    char                           Instruction = bf_get_instruction<Code, CheckPosition>::value>
struct bf_get_forward_jump_helper_finder
{
    static_assert(Instruction != 0, "Brainfuck Error: found [ without corresponding ]");
};

template<static_string_of_concept<char> Code, std::size_t CheckPosition, std::size_t StackSize, char Instruction>
    requires(Instruction != 0)
struct bf_get_forward_jump_helper_finder<Code, CheckPosition, StackSize, Instruction>
    : bf_get_forward_jump_helper_finder<Code, CheckPosition + 1, StackSize>
{
};

template<static_string_of_concept<char> Code, std::size_t CheckPosition, std::size_t StackSize>
struct bf_get_forward_jump_helper_finder<Code, CheckPosition, StackSize, '['>
    : bf_get_forward_jump_helper_finder<Code, CheckPosition + 1, StackSize + 1>
{
};

template<static_string_of_concept<char> Code, std::size_t CheckPosition, std::size_t StackSize>
struct bf_get_forward_jump_helper_finder<Code, CheckPosition, StackSize, ']'>
    : bf_get_forward_jump_helper_finder<Code, CheckPosition + 1, StackSize - 1>
{
};

template<static_string_of_concept<char> Code, std::size_t CheckPosition>
struct bf_get_forward_jump_helper_finder<Code, CheckPosition, 0, ']'>
    : std::integral_constant<std::size_t, CheckPosition + 1>
{
};

template<static_string_of_concept<char> Code, std::size_t InterpretPosition, static_string_of_concept<std::size_t> LoopStack, bool jumping>
struct bf_get_forward_jump_helper
{
    using loop_stack                           = typename LoopStack::template push_back<InterpretPosition>;
    static constexpr std::size_t next_position = InterpretPosition + 1;
};

template<static_string_of_concept<char> Code, std::size_t InterpretPosition, static_string_of_concept<std::size_t> LoopStack>
struct bf_get_forward_jump_helper<Code, InterpretPosition, LoopStack, true>
{
    using loop_stack = LoopStack;
    static constexpr std::size_t next_position = bf_get_forward_jump_helper_finder<Code, InterpretPosition + 1, 0>::value;
};

template<
    static_string_of_concept<char> Code,
    static_string_of_concept<char> Input,
    typename Memory,
    std::size_t                           InterpretPosition,
    std::size_t                           Ptr,
    static_string_of_concept<char>        Output,
    static_string_of_concept<std::size_t> LoopStack>
struct bf_interpreter<Code, Input, Memory, InterpretPosition, Ptr, Output, LoopStack, '['>
{
    static constexpr bool jump_forward = Memory::template get_value<Ptr> == 0;
    using jump_helper                  = bf_get_forward_jump_helper<Code, InterpretPosition, LoopStack, jump_forward>;

    using result =
        bf_interpreter<Code, Input, Memory, jump_helper::next_position, Ptr, Output, typename jump_helper::loop_stack>::result;
};

template<
    static_string_of_concept<char> Code,
    static_string_of_concept<char> Input,
    typename Memory,
    std::size_t                           InterpretPosition,
    std::size_t                           Ptr,
    static_string_of_concept<char>        Output,
    static_string_of_concept<std::size_t> LoopStack>
struct bf_interpreter<Code, Input, Memory, InterpretPosition, Ptr, Output, LoopStack, ']'>
{
    static constexpr bool jump_back = Memory::template get_value<Ptr> != 0;
    using result                    = bf_interpreter<
        Code,
        Input,
        Memory,
        (jump_back ? LoopStack::back : InterpretPosition + 1),
        Ptr,
        Output,
        typename LoopStack::template pop_back<>>::result;
};

template<typename CType, CType... Values>
constexpr auto operator""_bf()
{
    return typename bf_interpreter<basic_static_string<CType, Values...>>::result::output::create();
}

template<
    static_string_creation_concept Code,
    static_string_creation_concept Input,
    typename Interpreter = bf_interpreter<typename Code::PType, typename Input::PType>>
constexpr auto interpret_bf(Code, Input) -> Interpreter::result::output::create
{
    return {};
}

template<
    static_string_creation_concept Code,
    static_string_creation_concept Input,
    typename Interpreter = bf_interpreter<typename Code::PType, typename Input::PType>>
constexpr auto interpret_bf_ex(Code, Input)
{
    return std::pair<typename Interpreter::result::output::create, std::size_t>{{}, Interpreter::result::memory_usage};
}

int main(int argc, char** argv)
{
    // short "Hello, World!\n" from wikipedia: https://en.wikipedia.org/wiki/Brainfuck#Hello_World!
    // exhibits nested loops and as such requires increased template depth (makefile now has -ftemplate-depth=2048)
    constexpr auto result =
        "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++."_bf;
    static_assert(result.equals("Hello World!\n"_static));
    fmt::print("brainfuck result: {}", result.to_string_view());

    // more elaborate rot13 algorithm, also from wikipedia: https://en.wikipedia.org/wiki/Brainfuck#ROT13
    // exhibits even more nested loops and as such requires further increased template depth (makefile now has
    // -ftemplate-depth=32768
    // will still either OOM or hit that template-depth limitation with larger input strings
    constexpr auto rot13_bf =
        "-,+[-[>>++++[>++++++++<-]<+<-[>+>+>-[>>>]<[[>+<-]>>+>]<<<<<-]]>>>[-]+>--[-[<->+++[-]]]<[++++++++++++<[>-[>+>>]>[+[<+>-]>+>>]<<<<<-]>>[<+>-]>[-[-<<[-]>>]<<[<<->>-]>>]<<[<<+>>-]]<[-]<.[-]<-,+]"_static;

    // INPUT ALPHABET: ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
    // MAPPED OUTPUTS: NOPQRSTUVWXYZABCDEFGHIJKLMnopqrstuvwxyzabcdefghijklm
    // algorithm for, uh, reasons, requires a -1 as the terminator, so add \xFF to terminate the input
    constexpr auto rot13_input           = "ABCxyz\xFF"_static;
    constexpr auto rot13_expected_output = "NOPklm"_static;

    constexpr auto result_pair  = interpret_bf_ex(rot13_bf, rot13_input);
    constexpr auto input_result = result_pair.first;
    constexpr auto memory_usage = result_pair.second;

    static_assert(input_result.equals(rot13_expected_output));
    fmt::print("brainfuck input result: {} using {} bytes of memory\n", input_result.to_string_view(), memory_usage);
}