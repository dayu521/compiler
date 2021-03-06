#include "json.h"
#include "inner_imp.h"
namespace lsf {

Json::Json(const std::string &filename)
{
    //输入抽象
    buff_=std::make_shared<lsf::FilterBuff>(std::make_unique<lsf::MBuff>(filename));
    buff_->test_and_skipBOM();

    //创建词法分析器
    lexer_=std::make_shared<lsf::Lexer>(buff_);

    /************************/
    wrap_lexer_=std::make_shared<FunnyTokenGen>(lexer_,buff_);

    //创建语法分析器
    parser_=std::make_unique<lsf::JsonParser>(wrap_lexer_);

    //节点构建器
    builder=std::make_shared<Treebuilder>();

    parser_->set_builder(builder);
}

namespace detail {
template<typename F>
class Guard
{
public:
    Guard(F f):f_(f){};
    ~Guard()
    {
        try {
            f_();
        }  catch (...) {
            //不要抛异常
            //throw std::runtime_error("释放资源错误!");
//            std::cout<<std::current_exception()<<std::endl;
        }
    }
    F f_;
};
}

bool Json::run(std::function<void (ErrorType, const std::string &)> f)
{
    //setlocale(LC_ALL,"zh_CN.UTF-8");
    //std::locale("").name().c_str()

    //设置全局c++环境,所有之后std::locale()的实例都是此locale的副本，
    //同时设置本地c环境为用户偏好的locale，默认c环境的name好像是"C"
    //std::locale::global(std::locale(""));

    const auto old=std::setlocale(LC_ALL,nullptr);
    std::setlocale(LC_ALL,std::locale("").name().c_str());
    detail::Guard guard([old]{
        const auto restore = old;
#ifdef MSVC_SPECIAL
        restore = "C";
#endif // MSVC_SPECIAL
        std::setlocale(LC_ALL, restore);
    });
    error_msg_.clear();
    try {
        if(!parser_->parser()){
            error_msg_+=lsf::parser_messages(wrap_lexer_->token_position(),lexer_->get_token(),parser_->get_expect_token());
            f(ErrorType::ParserError,error_msg_);
            return false;
        }
    }  catch (const lsf::LexerError & e) {
        error_msg_+=lsf::lexer_messages(wrap_lexer_->token_position(),lexer_->get_token());
        f(ErrorType::LexError,error_msg_);
        return false;
    }   catch(const std::runtime_error &e){
        error_msg_+=e.what();
        f(ErrorType::UnknowError,error_msg_);
        return false;
    }
    return true;
}

bool Json::weak_type_check(std::function<void ( ErrorType et,const std::string & message)> f)
{
    error_msg_.clear();
    lsf::WeakTypeChecker typer;
    if(!typer.check_type(builder->get_ast())){
        error_msg_+="类型检查失败\n";
        error_msg_+=typer.get_error();
        error_msg_+='\n';
        f(ErrorType::WeakTypeCheckError,error_msg_);
        return false;
    }
    return true;
}

TreeNode *Json::get_output() const
{
    return std::get<0>(builder->get_ast());
}

std::string Json::get_errors() const
{
    return error_msg_;
}

}
