#pragma once

#include <array>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

template<typename CType, CType... Values>
struct basic_static_string;

template<char... Values>
using static_string = basic_static_string<char, Values...>;

template<typename T>
struct is_static_string : std::false_type
{
};

template<typename CType, CType... Values>
struct is_static_string<basic_static_string<CType, Values...>> : std::true_type
{
};

template<typename T, typename CType>
struct is_static_string_of : std::false_type
{
};

template<typename CType, CType... Values>
struct is_static_string_of<basic_static_string<CType, Values...>, CType> : std::true_type
{
};

template<typename T>
concept static_string_concept = is_static_string<T>::value;

template<typename T, typename CType>
concept static_string_of_concept = is_static_string_of<T, CType>::value;

template<typename T>
concept static_string_creation_concept = requires(T a) {
                                             typename T::PType;
                                             requires(static_string_concept<typename T::PType>);
                                         };

template<typename T, typename CType>
concept static_string_creation_of_concept = requires(T a) {
                                                typename T::PType;
                                                requires(static_string_of_concept<typename T::PType, CType>);
                                            };

template<std::size_t I, typename CType, CType... Values>
struct static_string_at : std::integral_constant<CType, std::array<CType, sizeof...(Values)>{Values...}[I]>
{
    static_assert(I < sizeof...(Values), "static_string_at index not within range");
};

template<static_string_concept T, typename Indices, std::size_t Offset = 0>
struct static_string_gather_by_indices
{
};

template<static_string_concept T, std::size_t Offset, std::size_t... Is>
struct static_string_gather_by_indices<T, std::index_sequence<Is...>, Offset>
{
    using type = basic_static_string<typename T::char_type, T::template at<Offset + Is>...>;
};

template<static_string_concept T, std::size_t RemovePrefix>
struct static_string_remove_prefix
{
    static_assert(T::size >= RemovePrefix);
    using index_sequence = std::make_index_sequence<T::size - RemovePrefix>;
    using type           = static_string_gather_by_indices<T, index_sequence, RemovePrefix>::type;
};

template<static_string_concept T, std::size_t RemoveSuffix>
struct static_string_remove_suffix
{
    static_assert(T::size >= RemoveSuffix);
    using index_sequence = std::make_index_sequence<T::size - RemoveSuffix>;
    using type           = static_string_gather_by_indices<T, index_sequence>::type;
};

template<static_string_concept T, std::size_t Start, std::size_t Length>
struct static_string_substr
{
    static_assert(T::size >= Start);
    static constexpr auto ActualLength = (Length == T::npos || (Length > (T::size - Start))) ? (T::size - Start) : Length;
    using index_sequence = std::make_index_sequence<ActualLength>;
    using type           = static_string_gather_by_indices<T, index_sequence, Start>::type;
};

template<static_string_concept Haystack, static_string_of_concept<typename Haystack::char_type> Needle>
struct static_string_index_of
    : std::integral_constant<std::size_t, Haystack::to_string_view().find(Needle::to_string_view())>
{
};

/*template<static_string_concept Haystack, static_string_of_concept<typename Haystack::char_type> Needle>
struct static_string_index_of : std::integral_constant<std::size_t, Haystack::npos>
{
};

template<std::size_t I>
struct static_string_maybe_inc_index : std::integral_constant<std::size_t, (I == std::string_view::npos ? I : I + 1)>
{
};

template<static_string_concept Haystack, static_string_of_concept<typename Haystack::char_type> Needle>
    requires(Needle::size <= Haystack::size)
struct static_string_index_of<Haystack, Needle>
    : std::integral_constant<
          std::size_t,
          (Haystack::template starts_with<Needle>
               ? 0
               : static_string_maybe_inc_index<
                     static_string_index_of<typename Haystack::template remove_prefix<1>, Needle>::value>::value)>
{
};*/

template<
    static_string_concept                             Str,
    static_string_of_concept<typename Str::char_type> Find,
    static_string_of_concept<typename Str::char_type> Replace>
struct static_string_find_and_replace
{
    static constexpr auto pos = Str::template find<Find>;
    using type =
        std::conditional_t<pos == Str::npos, Str, typename Str::template erase<pos, Find::size>::template insert<pos, Replace>>;
};

template<
    static_string_concept                             Str,
    static_string_of_concept<typename Str::char_type> Find,
    static_string_of_concept<typename Str::char_type> Replace,
    typename = void>
struct static_string_find_and_replace_all
{
    using type = Str;
};

template<
    static_string_concept                             Str,
    static_string_of_concept<typename Str::char_type> Find,
    static_string_of_concept<typename Str::char_type> Replace>
struct static_string_find_and_replace_all<Str, Find, Replace, std::enable_if_t<(Str::template find<Find> != Str::npos)>>
{
    static constexpr auto pos = Str::template find<Find>;
    using type                = std::conditional_t<
        pos == Str::npos,
        Str,
        typename static_string_find_and_replace_all<
            typename Str::template erase<pos, Find::size>::template insert<pos, Replace>,
            Find,
            Replace>::type>;
};

template<typename CType, CType... Values>
struct static_string_data
{
    static constexpr CType m_data[] = {Values...};
};

template<typename CType, CType... Values>
struct basic_static_string
{
    using Self                        = basic_static_string;
    using char_type                   = CType;
    static constexpr std::size_t size = sizeof...(Values);
    static constexpr std::size_t npos = std::basic_string_view<CType>::npos;

    static constexpr auto to_string_view() -> std::basic_string_view<CType>
    {
        // specialization only created if `basic_static_string::to_string_view` is called for a given
        // `basic_static_string` this must be the case as methods of a template are only instantiated
        // when used, else SFINAE tricks like using std::enable_if wouldn't work to disable methods
        // for certain instantiations
        return std::basic_string_view<CType>{static_string_data<CType, Values...>::m_data, sizeof...(Values)};
    }

    template<std::size_t I>
        requires(I < size)
    static constexpr CType at = static_string_at<I, CType, Values...>::value;

    template<CType... AdditionalValues>
    using append_chars = basic_static_string<CType, Values..., AdditionalValues...>;

    template<CType... AdditionalValues>
    using prepend_chars = basic_static_string<CType, AdditionalValues..., Values...>;

    template<static_string_of_concept<CType> T>
    using append = typename T::template applied_to<append_chars>;

    template<static_string_of_concept<CType> T>
    using prepend = typename T::template applied_to<prepend_chars>;

    template<std::size_t Count>
        requires(Count <= size)
    using remove_prefix = typename static_string_remove_prefix<Self, Count>::type;

    template<std::size_t Count>
        requires(Count <= size)
    using remove_suffix = typename static_string_remove_suffix<Self, Count>::type;

    template<std::size_t Count>
    using trim_to = remove_suffix<(Count < size ? size - Count : 0)>;

    template<std::size_t Count>
    using reverse_trim_to = remove_prefix<(Count < size ? size - Count : 0)>;

    template<CType V>
    using push_front = basic_static_string<CType, V, Values...>;

    template<CType V>
    using push_back = basic_static_string<CType, Values..., V>;

    template<std::size_t Count = 1>
    using pop_front = remove_prefix<Count>;

    template<std::size_t Count = 1>
    using pop_back = remove_suffix<Count>;

    static constexpr CType front = at<0>;
    static constexpr CType back  = at<size - 1>;

    template<template<CType...> typename TargetType>
    using applied_to = TargetType<Values...>;

    template<std::size_t Start, std::size_t Length>
        requires((Start + Length) <= size)
    using substr = typename static_string_substr<Self, Start, Length>::type;

    template<std::size_t I, std::size_t Count = 1>
        requires(I < size && (I + Count) <= size)
    using erase = typename substr<0, I>::template append<substr<(I + Count), ((size - I) - Count)>>;

    template<std::size_t I, static_string_of_concept<CType> T>
    using insert = typename substr<0, I>::template append<T>::template append<substr<I, (size - I)>>;

    template<static_string_of_concept<CType> Find, static_string_of_concept<CType> Replace>
    using find_and_replace = static_string_find_and_replace<Self, Find, Replace>::type;

    template<static_string_of_concept<CType> Find, static_string_of_concept<CType> Replace>
    using find_and_replace_all = static_string_find_and_replace_all<Self, Find, Replace>::type;

    template<std::size_t I, CType V>
    using replace = substr<0, I>::template append_chars<V>::template append<substr<I + 1, size - (I + 1)>>;

    using to_lower = basic_static_string<
        CType,
        ((Values >= static_cast<CType>('A') && Values <= static_cast<CType>('Z'))
             ? Values + (static_cast<CType>('a') - static_cast<CType>('A'))
             : Values)...>;
    using to_upper = basic_static_string<
        CType,
        ((Values >= static_cast<CType>('a') && Values <= static_cast<CType>('z'))
             ? Values - (static_cast<CType>('a') - static_cast<CType>('A'))
             : Values)...>;

    template<CType Value>
    static constexpr std::size_t count_of = (0 + ... + (Values == Value ? 1 : 0));

    template<static_string_of_concept<CType> T>
    static constexpr bool equals = std::is_same<T, Self>::value;

    template<static_string_of_concept<CType> T>
    static constexpr bool equals_case_insensitive = std::is_same<typename T::to_lower, to_lower>::value;

    template<static_string_of_concept<CType> T>
    static constexpr bool starts_with = trim_to<T::size>::template equals<T>;

    template<static_string_of_concept<CType> T>
    static constexpr bool starts_with_case_insensitive = trim_to<T::size>::template equals_case_insensitive<T>;

    template<static_string_of_concept<CType> T>
    static constexpr bool ends_with = reverse_trim_to<T::size>::template equals<T>;

    template<static_string_of_concept<CType> T>
    static constexpr bool ends_with_case_insensitive = reverse_trim_to<T::size>::template equals_case_insensitive<T>;

    template<static_string_of_concept<CType> T>
    static constexpr std::size_t find = static_string_index_of<Self, T>::value;

    template<static_string_of_concept<CType> T>
    static constexpr bool contains = find<T> != npos;

    struct create
    {
        using PType = Self;

        static constexpr auto npos = PType::npos;

        constexpr auto size() const -> std::size_t { return PType::size; }

        constexpr auto to_string_view() const -> std::basic_string_view<CType> { return PType::to_string_view(); }

        template<std::size_t I>
        constexpr auto at() const -> CType
        {
            return PType::at<I>;
        }

        template<static_string_creation_of_concept<CType> T>
        constexpr auto append(T) const -> PType::append<typename T::PType>::create
        {
            return {};
        }

        template<static_string_creation_of_concept<CType> T>
        constexpr auto prepend(T) const -> PType::prepend<typename T::PType>::create
        {
            return {};
        }

        template<std::size_t Count>
        constexpr auto remove_prefix() const -> PType::remove_prefix<Count>::create
        {
            return {};
        }

        template<std::size_t Count>
        constexpr auto remove_suffix() const -> PType::remove_suffix<Count>::create
        {
            return {};
        }

        template<std::size_t Count>
        constexpr auto trim_to() const -> PType::trim_to<Count>::create
        {
            return {};
        }

        template<std::size_t Count>
        constexpr auto reverse_trim_to() const -> PType::reverse_trim_to<Count>::create
        {
            return {};
        }

        template<CType V>
        constexpr auto push_front() const -> PType::push_front<V>::create
        {
            return {};
        }

        template<CType V>
        constexpr auto push_back() const -> PType::push_back<V>::create
        {
            return {};
        }

        constexpr auto pop_front() const -> PType::pop_front<>::create { return {}; }
        constexpr auto pop_back() const -> PType::pop_back<>::create { return {}; }

        constexpr auto front() const -> CType { return PType::front; }
        constexpr auto back() const -> CType { return PType::back; }

        template<std::size_t Start, std::size_t Length>
        constexpr auto substr() const -> PType::substr<Start, Length>::create
        {
            return {};
        }

        template<std::size_t I, std::size_t Count>
        constexpr auto erase() const -> PType::erase<I, Count>::create
        {
            return {};
        }

        template<std::size_t I, static_string_creation_of_concept<CType> T>
        constexpr auto insert(T) const -> PType::insert<I, typename T::PType>::create
        {
            return {};
        }

        template<static_string_creation_of_concept<CType> Find, static_string_creation_of_concept<CType> Replace>
        constexpr auto find_and_replace(Find, Replace) const
            -> find_and_replace<typename Find::PType, typename Replace::PType>::create
        {
            return {};
        }

        template<static_string_creation_of_concept<CType> Find, static_string_creation_of_concept<CType> Replace>
        constexpr auto find_and_replace_all(Find, Replace) const
            -> find_and_replace_all<typename Find::PType, typename Replace::PType>::create
        {
            return {};
        }

        template<std::size_t I, CType V>
        constexpr auto replace() const -> PType::replace<I, V>
        {
            return {};
        }

        constexpr auto to_lower() const -> PType::to_lower::create { return {}; }

        constexpr auto to_upper() const -> PType::to_upper::create { return {}; }

        constexpr auto count_of(CType c) const -> std::size_t { return (0 + ... + (Values == c ? 1 : 0)); }

        template<static_string_creation_of_concept<CType> T>
        constexpr auto equals(T) const -> bool
        {
            return PType::equals<typename T::PType>;
        }

        template<static_string_creation_of_concept<CType> T>
        constexpr auto equals_case_insensitive(T) const -> bool
        {
            return PType::equals_case_insensitive<typename T::PType>;
        }

        template<static_string_creation_of_concept<CType> T>
        constexpr auto starts_with(T) const -> bool
        {
            return PType::starts_with<typename T::PType>;
        }

        template<static_string_creation_of_concept<CType> T>
        constexpr auto starts_with_case_insensitive(T) const -> bool
        {
            return PType::starts_with_case_insensitive<typename T::PType>;
        }

        template<static_string_creation_of_concept<CType> T>
        constexpr auto ends_with(T) const -> bool
        {
            return PType::ends_with<typename T::PType>;
        }

        template<static_string_creation_of_concept<CType> T>
        constexpr auto ends_with_case_insensitive(T) const -> bool
        {
            return PType::ends_with_case_insensitive<typename T::PType>;
        }

        template<static_string_creation_of_concept<CType> T>
        constexpr auto find(T) const -> std::size_t
        {
            return PType::find<typename T::PType>;
        }

        template<static_string_creation_of_concept<CType> T>
        constexpr auto contains(T) const -> bool
        {
            return PType::contains<typename T::PType>;
        }
    };
};

template<typename CType, CType... Values>
consteval basic_static_string<CType, Values...>::create operator"" _static()
{
    return {};
}

template<typename CType, typename T>
struct make_static_string_for
{
};

template<typename CType, std::size_t... Is>
struct make_static_string_for<CType, std::index_sequence<Is...>>
{
    using type = basic_static_string<CType, (Is, 0)...>;
};

template<typename CType, std::size_t N>
using make_static_string = make_static_string_for<CType, std::make_index_sequence<N>>::type;

#define STATIC_STRING(str)                                                                                                 \
    decltype([]<std::size_t... Is>(std::index_sequence<Is...>) {                                                           \
        return basic_static_string<std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(str)>>>, str[Is]...>{}; \
    }(std::make_index_sequence<sizeof(str) - 1>{}))

template<std::size_t... Is, typename Return = basic_static_string<char, std::source_location::current().function_name()[Is]...>::create>
consteval auto parse_current_function_to_static_string(std::index_sequence<Is...>)
{
    return Return{};
}

template<
    typename... Tags,
    auto Size   = std::string_view{std::source_location::current().function_name()}.size(),
    auto Return = parse_current_function_to_static_string(std::make_index_sequence<Size>{})>
constexpr auto get_current_function()
{
    return Return;
}
template<
    auto FirstTag,
    auto... Tags,
    auto Size   = std::string_view{std::source_location::current().function_name()}.size(),
    auto Return = parse_current_function_to_static_string(std::make_index_sequence<Size>{})>
constexpr auto get_current_function()
{
    return Return;
}

#define MORE_PORTABLE
#ifdef MORE_PORTABLE
#define GET_CURRENT_FUNCTION_T return get_current_function<T>()
#define GET_CURRENT_FUNCTION_V return get_current_function<V>()
#else
#define GET_CURRENT_FUNCTION_T               \
    constexpr auto& f = __PRETTY_FUNCTION__; \
    return typename STATIC_STRING(f)::create()
#define GET_CURRENT_FUNCTION_V               \
    constexpr auto& f = __PRETTY_FUNCTION__; \
    return typename STATIC_STRING(f)::create()
#endif

template<typename T>
consteval auto get_function_name()
{
    GET_CURRENT_FUNCTION_T;
}

template<auto V>
consteval auto get_function_name()
{
    GET_CURRENT_FUNCTION_V;
}

struct typename_helper
{
    struct XXX_TYPENAME_FINDER_TAG_XXX
    {
    };

    static constexpr auto        needle      = "typename_helper::XXX_TYPENAME_FINDER_TAG_XXX"_static;
    static constexpr auto        finder_tag  = get_function_name<XXX_TYPENAME_FINDER_TAG_XXX>();
    static constexpr std::size_t start_index = finder_tag.find(needle);
    static constexpr std::size_t end_index   = start_index + needle.size();
    static constexpr std::size_t extra_chars = finder_tag.size() - end_index;

    static_assert(start_index != finder_tag.npos);
};

template<typename T>
consteval auto get_typename()
{
    constexpr auto func_name = get_function_name<T>();
    return func_name.template remove_prefix<typename_helper::start_index>()
        .template remove_suffix<typename_helper::extra_chars>();
}

struct value_name_helper
{
    enum class value_finder_helper
    {
        XXX_VALUE_FINDER_TAG_XXX
    };

    static constexpr auto        needle     = "value_name_helper::value_finder_helper::XXX_VALUE_FINDER_TAG_XXX"_static;
    static constexpr auto        finder_tag = get_function_name<value_finder_helper::XXX_VALUE_FINDER_TAG_XXX>();
    static constexpr std::size_t start_index = finder_tag.find(needle);
    static constexpr std::size_t end_index   = start_index + needle.size();
    static constexpr std::size_t extra_chars = finder_tag.size() - end_index;

    static_assert(start_index != finder_tag.npos);
};

template<auto V>
consteval auto get_value_name()
{
    constexpr auto func_name = get_function_name<V>();
    return func_name.template remove_prefix<value_name_helper::start_index>()
        .template remove_suffix<value_name_helper::extra_chars>();
}

template<typename T>
using typename_for = decltype(get_typename<T>())::PType;

template<auto V>
using name_for = decltype(get_value_name<V>())::PType;

#ifdef TEST_STATIC_STRING
namespace impl_test
{
struct TestType
{
};
struct Another_Type_For_Testing
{
};

constexpr auto tt_name = typename_for<TestType>::create{};

static_assert(get_typename<TestType>().equals("impl_test::TestType"_static));
static_assert(get_typename<Another_Type_For_Testing>().equals("impl_test::Another_Type_For_Testing"_static));
static_assert(tt_name.to_string_view() == "impl_test::TestType");
static_assert(tt_name.prepend("derp::"_static).equals("derp::impl_test::TestType"_static));
static_assert(tt_name.append("::derp"_static).equals("impl_test::TestType::derp"_static));
static_assert(tt_name.remove_prefix<5>().equals("test::TestType"_static));
static_assert(tt_name.remove_suffix<4>().equals("impl_test::Test"_static));
static_assert(tt_name.trim_to<4>().equals("impl"_static));
static_assert(tt_name.reverse_trim_to<4>().equals("Type"_static));
static_assert(tt_name.substr<4, 5>().equals("_test"_static));
static_assert(tt_name.erase<4, 5>().equals("impl::TestType"_static));
static_assert(tt_name.insert<4>("_derp"_static).equals("impl_derp_test::TestType"_static));
static_assert(tt_name.find_and_replace(":"_static, "?"_static).equals("impl_test?:TestType"_static));
static_assert(tt_name.find_and_replace_all(":"_static, "?"_static).equals("impl_test??TestType"_static));
static_assert(tt_name.to_lower().equals("impl_test::testtype"_static));
static_assert(tt_name.to_upper().equals("IMPL_TEST::TESTTYPE"_static));
static_assert(tt_name.at<2>() == 'p');
static_assert(tt_name.count_of(':') == 2);
static_assert(tt_name.equals("impl_test::TestType"_static));
static_assert(tt_name.equals_case_insensitive("IMPL_TEST::TESTTYPE"_static));
static_assert(tt_name.starts_with("impl_test::"_static));
static_assert(tt_name.starts_with_case_insensitive("ImPl_TeSt::"_static));
static_assert(tt_name.ends_with("TestType"_static));
static_assert(tt_name.ends_with_case_insensitive("TeStTyPe"_static));
static_assert(tt_name.find("Type"_static) == 15); // impl_test::Test*
static_assert(tt_name.contains("::"_static));
} // namespace impl_test
#endif
