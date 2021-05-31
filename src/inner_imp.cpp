#include<cassert>
#include "inner_imp.h"
#include "lexer.h"
#include <limits>

namespace lsf {

KMP::KMP(const std::wstring &p_):pattern(p_)
{
    pattern_len=pattern.size();
    pi.resize(pattern_len);
    piFunc();
}

int KMP::match(const std::wstring &str) const
{
    int k=pi[0];
    for(std::size_t i=0;i<str.size();i++){
        while(k>0&&pattern[k]!=str[i])
            k=pi[k-1];
        if(pattern[k]==str[i])
            k++;
        if(k==pattern_len){
//            using std::wcout;
//            wcout<<str.substr(0,i-pattern_len+1)<<L"("<<
//                  pattern<<L")"<<str.substr(i+1)<<std::endl;
//            k=pi[k];
//            break;
            k=pi[0];
        }
    }
    return str.size()-k;
}

KMP::~KMP()
{
}

void KMP::piFunc()
{
    pi[0]=0;
    int k=pi[0];
    for(int i=1;i<pattern_len;i++){
        while(k>0&&pattern[k]!=pattern[i]){
            k=pi[k-1];
        }
        if(pattern[k]==pattern[i]){
            k++;
        }
        pi[i]=k;
    }
//    for(int i=0;i<pattern_len;i++)
//        std::cout<<pi[i]<<" ";
//    std::cout<<std::endl;
}

FilterBuff::FilterBuff(std::unique_ptr<BuffBase> buff):b_(std::move(buff)),history_(1,1),p_begin_{1,1}
{
}

FilterBuff::~FilterBuff()
{

}

wchar_t FilterBuff::next_char()
{
    auto c=b_->next_char();
    number_++;
    if(c==L'\n'){
        //    \n \n  换行符
        //  5  2  1     数量
        //  0  1  2     数组索引
        //除第一次外，每次遇到换行就从数组下一位开始从1计数，直到遇到换行符，然后重复过程，数组的大小减一就是当前token的换行符个数
        //在回滚的时候从后往前，依次减掉回滚的个数，然后遇到换行符就把stat_.line_减一
        //所以上述图示表示,前5个字符不是换行符，第6个字符是换行符，第7个字符不是换行符，第8个是换行符
        //好吧 我描述的不太好，仔细想想就明白了
        history_.push_back(0);
    }
    history_.back()++;
    return c;
}

///这个函数实现有毒,我跪了
void FilterBuff::rollback_char(std::size_t len)
{
    if(number_<=len){
        this->rollback_all_chars();
        return;
    }
    number_-=len;
    b_->rollback_char(len);
    auto n=history_.size();
    long llen=len;
    assert(len <= std::numeric_limits<decltype(llen)>::max());
    do{
        n--;
        llen-=history_[n];  //不会出现llen=0&&n=0,这种情况属于上面的if的责任
    }while(llen>=0);
    //history_[n]就是stat_.column_curr_
    history_[n]=-llen;
    //实际上不用判断，直接操作，但一般来说，回滚只是一两个字符，
    //所以当n仅仅减少1时，直接跳过,不需要操作
    if(n<history_.size()-1){
        history_.resize(n+1);
    }
}

void FilterBuff::rollback_all_chars()
{
    b_->rollback_all_chars();
    number_=0;

    history_.resize(1);
    history_[0]=p_begin_.column_;
}

///和get_token()功能几乎相同,但丢弃缓冲区内的字符(隐含着当前处理完成,准备分析下个token)
void FilterBuff::discard_token()
{
    b_->discard_token();
}

///获取当前缓冲区的字符(假定它们被分析,且已经被认为是token了)
std::wstring FilterBuff::get_token()
{
    return b_->get_token();
}

Location FilterBuff::begin_location()
{
    return p_begin_;
}

void FilterBuff::record_location()
{
    number_=0;
    auto old_size=history_.size();
    //初始化history_就是初始化p_current_
    history_[0]=history_.back();
    history_.resize(1);

    p_begin_.column_=history_.back();
    p_begin_.line_+=old_size-1;
}

///这应该成为成员函数吗?
bool FilterBuff::test_and_skipBOM()
{
    wchar_t head[3]={};
    for (std::size_t i=0;i<3;i++){
        head[i]=b_->next_char();
        if(head[i]==MBuff::Eof_w)
            return true;
    }
    if(wcsncmp(head,L"\xEF\xBB\xBF",3)==0){
        b_->discard_token();
        return true;
    }
    b_->rollback_char(3);
    return false;
}

///*************************///
Location FunnyTokenGen::token_position() const
{
    return tk_begin_;
}

void FunnyTokenGen::next_()
{
    buff_->record_location();
    tk_begin_=buff_->begin_location();
    lexer_->next_token();
}

Token &FunnyTokenGen::current_()
{
    return lexer_->get_token();
}

}