#pragma once

#include <pl/formatters/formatter.hpp>
#include <pl/patterns/pattern_error.hpp>

namespace pl::gen::fmt {

    class XmlPatternVisitor : public FormatterPatternVisitor {
    public:
        XmlPatternVisitor() = default;
        ~XmlPatternVisitor() override = default;

        void visit(pl::ptrn::PatternArrayDynamic& pattern)  override { formatArray(&pattern);       }
        void visit(pl::ptrn::PatternArrayStatic& pattern)   override { formatArray(&pattern);       }
        void visit(pl::ptrn::PatternBitfieldField& pattern) override { formatValue(&pattern);       }
        void visit(pl::ptrn::PatternBitfieldArray& pattern) override { formatArray(&pattern);       }
        void visit(pl::ptrn::PatternBitfield& pattern)      override { formatObject(&pattern);      }
        void visit(pl::ptrn::PatternBoolean& pattern)       override { formatValue(&pattern);       }
        void visit(pl::ptrn::PatternCharacter& pattern)     override { formatString(&pattern);      }
        void visit(pl::ptrn::PatternEnum& pattern)          override { formatString(&pattern);      }
        void visit(pl::ptrn::PatternFloat& pattern)         override { formatValue(&pattern);       }
        void visit(pl::ptrn::PatternPadding& pattern)       override { wolv::util::unused(pattern); }
        void visit(pl::ptrn::PatternPointer& pattern)       override { formatPointer(&pattern);     }
        void visit(pl::ptrn::PatternSigned& pattern)        override { formatValue(&pattern);       }
        void visit(pl::ptrn::PatternString& pattern)        override { formatString(&pattern);      }
        void visit(pl::ptrn::PatternStruct& pattern)        override { formatObject(&pattern);      }
        void visit(pl::ptrn::PatternUnion& pattern)         override { formatObject(&pattern);      }
        void visit(pl::ptrn::PatternUnsigned& pattern)      override { formatValue(&pattern);       }
        void visit(pl::ptrn::PatternWideCharacter& pattern) override { formatString(&pattern);      }
        void visit(pl::ptrn::PatternWideString& pattern)    override { formatString(&pattern);      }
        void visit(pl::ptrn::PatternError& pattern)         override { formatString(&pattern);      }
        void visit(pl::ptrn::Pattern& pattern)              override { formatString(&pattern);      }

        [[nodiscard]] auto getResult() const {
            return this->m_result;
        }

        void pushIndent() {
            this->m_indent += 4;
        }

        void popIndent() {
            this->m_indent -= 4;
        }

    private:
        void addLine(const std::string &variableName, const std::string& str, bool noVariableName = false) {
            this->m_result += std::string(this->m_indent, ' ');
            
            if (!str.empty())
                this->m_result += str;
                
            this->m_result += '\n';
        }

        void formatString(const pl::ptrn::Pattern *pattern) {
            if (pattern->getVisibility() == ptrn::Visibility::Hidden) return;
            if (pattern->getVisibility() == ptrn::Visibility::TreeHidden) return;

            const auto string = pattern->toString();

            std::string result;
            for (char c : string) {
                if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
                    result += c;
                else
                    result += ::fmt::format("%{:02X}", c);
            }

            std::string xmlElement = ::fmt::format("<{0}>{1}</{0}>", 
                pattern->getVariableName(), 
                result);
                
            addLine(pattern->getVariableName(), xmlElement);
        }

        template<typename T>
        void formatArray(T *pattern) {
            if (pattern->getVisibility() == ptrn::Visibility::Hidden) return;
            if (pattern->getVisibility() == ptrn::Visibility::TreeHidden) return;

            addLine("", ::fmt::format("<{} type=\"array\">", pattern->getVariableName()));
            pushIndent();
            
            // Add meta information
            if (this->isMetaInformationEnabled()) {
                addLine("", "<meta>");
                pushIndent();
                
                for (const auto &[name, value] : this->getMetaInformation(pattern))
                    addLine("", ::fmt::format("<{0}>{1}</{0}>", name, value));
                
                popIndent();
                addLine("", "</meta>");
            }
            
            // Add elements
            addLine("", "<elements>");
            pushIndent();
            pattern->forEachEntry(0, pattern->getEntryCount(), [&](u64 index, auto member) {
                addLine("", ::fmt::format("<element index=\"{}\">", index));
                pushIndent();
                member->accept(*this);
                popIndent();
                addLine("", "</element>");
            });
            popIndent();
            addLine("", "</elements>");
            
            popIndent();
            addLine("", ::fmt::format("</{}>", pattern->getVariableName()));
        }

        void formatPointer(ptrn::PatternPointer *pattern) {
            if (pattern->getVisibility() == ptrn::Visibility::Hidden) return;
            if (pattern->getVisibility() == ptrn::Visibility::TreeHidden) return;

            addLine("", ::fmt::format("<{} type=\"pointer\">", pattern->getVariableName()));
            pushIndent();
            
            // Add meta information
            if (this->isMetaInformationEnabled()) {
                addLine("", "<meta>");
                pushIndent();
                
                for (const auto &[name, value] : this->getMetaInformation(pattern))
                    addLine("", ::fmt::format("<{0}>{1}</{0}>", name, value));
                
                popIndent();
                addLine("", "</meta>");
            }
            
            // Add pointed content
            addLine("", "<pointed_content>");
            pushIndent();
            pattern->getPointedAtPattern()->accept(*this);
            popIndent();
            addLine("", "</pointed_content>");
            
            popIndent();
            addLine("", ::fmt::format("</{}>", pattern->getVariableName()));
        }

        template<typename T>
        void formatObject(T *pattern) {
            if (pattern->getVisibility() == ptrn::Visibility::Hidden) return;
            if (pattern->getVisibility() == ptrn::Visibility::TreeHidden) return;

            if (pattern->isSealed()) {
                formatValue(pattern);
            } else {
                addLine("", ::fmt::format("<{} type=\"object\">", pattern->getVariableName()));
                pushIndent();
                
                // Add meta information
                if (this->isMetaInformationEnabled()) {
                    addLine("", "<meta>");
                    pushIndent();
                    
                    for (const auto &[name, value] : this->getMetaInformation(pattern))
                        addLine("", ::fmt::format("<{0}>{1}</{0}>", name, value));
                    
                    popIndent();
                    addLine("", "</meta>");
                }
                
                // Add members
                addLine("", "<members>");
                pushIndent();
                pattern->forEachEntry(0, pattern->getEntryCount(), [&](u64, auto member) {
                    member->accept(*this);
                });
                popIndent();
                addLine("", "</members>");
                
                popIndent();
                addLine("", ::fmt::format("</{}>", pattern->getVariableName()));
            }
        }

        void formatValue(const pl::ptrn::Pattern *pattern) {
            if (pattern->getVisibility() == ptrn::Visibility::Hidden) return;
            if (pattern->getVisibility() == ptrn::Visibility::TreeHidden) return;

            if (const auto &functionName = pattern->getReadFormatterFunction(); !functionName.empty())
                formatString(pattern);
            else if (!pattern->isSealed()) {
                auto literal = pattern->getValue();

                std::string value = std::visit(wolv::util::overloaded {
                    [&](integral auto val)            -> std::string { return ::fmt::format("{}", val); },
                    [&](std::floating_point auto val) -> std::string { return ::fmt::format("{}", val); },
                    [&](const std::string &val)       -> std::string { return val; },
                    [&](bool val)                     -> std::string { return val ? "true" : "false"; },
                    [&](char val)                     -> std::string { return ::fmt::format("{}", val); },
                    [&](const std::shared_ptr<ptrn::Pattern> &val) -> std::string { return val->toString(); },
                }, literal);
                
                std::string xmlElement = ::fmt::format("<{0}>{1}</{0}>", 
                    pattern->getVariableName(), 
                    value);
                    
                addLine(pattern->getVariableName(), xmlElement);
            }
        }

    private:
        std::string m_result;
        u32 m_indent = 0;
    };

    class FormatterXml : public Formatter {
    public:
        FormatterXml() : Formatter("xml") { }
        ~FormatterXml() override = default;

        [[nodiscard]] std::string getFileExtension() const override { return "xml"; }

        [[nodiscard]] std::vector<u8> format(const PatternLanguage &runtime) override {
            XmlPatternVisitor visitor;
            visitor.enableMetaInformation(this->isMetaInformationEnabled());

            // XML header
            std::string result = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            result += "<pattern_language>\n";
            
            // Process patterns
            visitor.pushIndent();
            for (const auto& pattern : runtime.getPatterns()) {
                pattern->accept(visitor);
            }
            visitor.popIndent();
            
            // Add visitor result
            result += visitor.getResult();
            
            // Close root element
            result += "</pattern_language>";

            return { result.begin(), result.end() };
        }
    };

}