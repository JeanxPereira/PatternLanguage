#pragma once

#include <pl/formatters/formatter.hpp>
#include <hex/helpers/fmt.hpp>
#include <pl/core/ast/ast_node.hpp>

namespace pl::gen::fmt {

    class FormatterXml : public Formatter {
    public:
        FormatterXml() : Formatter("xml") { }
        ~FormatterXml() override = default;

        [[nodiscard]] std::string getFileExtension() const override { return "xml"; }

        [[nodiscard]] std::vector<u8> format(const PatternLanguage &runtime) override {
            auto result = generateXml(runtime);

            return { result.begin(), result.end() };
        }

    private:
        class XmlVisitor : public FormatterPatternVisitor {
        public:
            explicit XmlVisitor(const PatternLanguage &runtime) : m_runtime(runtime) { }

            void visit(ptrn::PatternArrayDynamic *pattern) override {
                processPattern(pattern, [this, pattern]() {
                    for (u64 i = 0; i < pattern->getEntryCount(); i++) {
                        auto &entry = pattern->getEntry(i);
                        
                        m_output += ::fmt::format("{}<entry index=\"{}\">", indent(m_depth + 1), i);
                        
                        m_depth++;
                        entry->accept(*this);
                        m_depth--;
                        
                        m_output += ::fmt::format("{}</entry>\n", indent(m_depth + 1));
                    }
                });
            }

            void visit(ptrn::PatternArrayStatic *pattern) override {
                processPattern(pattern, [this, pattern]() {
                    for (u64 i = 0; i < pattern->getEntryCount(); i++) {
                        auto &entry = pattern->getEntry(i);
                        
                        m_output += ::fmt::format("{}<entry index=\"{}\">", indent(m_depth + 1), i);
                        
                        m_depth++;
                        entry->accept(*this);
                        m_depth--;
                        
                        m_output += ::fmt::format("{}</entry>\n", indent(m_depth + 1));
                    }
                });
            }

            void visit(ptrn::PatternBitfield *pattern) override {
                processPattern(pattern, [this, pattern]() {
                    for (auto &[name, bitfield] : pattern->getBitfields()) {
                        m_output += ::fmt::format("{}<field name=\"{}\">", indent(m_depth + 1), escapeXml(name));
                        
                        m_depth++;
                        bitfield.pattern->accept(*this);
                        m_depth--;
                        
                        m_output += ::fmt::format("{}</field>\n", indent(m_depth + 1));
                    }
                });
            }

            void visit(ptrn::PatternBoolean *pattern) override {
                processPattern(pattern, [pattern]() {
                    return ::fmt::format("{}", pattern->getValue() ? "true" : "false");
                });
            }

            void visit(ptrn::PatternCharacter *pattern) override {
                processPattern(pattern, [pattern]() {
                    auto value = pattern->getValue();
                    if (std::isprint(value))
                        return ::fmt::format("'{}'", escapeXml(std::string(1, value)));
                    else
                        return ::fmt::format("{}", value);
                });
            }

            void visit(ptrn::PatternEnum *pattern) override {
                processPattern(pattern, [pattern]() {
                    return ::fmt::format("{}", escapeXml(pattern->getFormattedValue()));
                });
            }

            void visit(ptrn::PatternFloat *pattern) override {
                processPattern(pattern, [pattern]() {
                    return ::fmt::format("{}", pattern->getValue());
                });
            }

            void visit(ptrn::PatternPadding *pattern) override {
                processPattern(pattern, [pattern]() {
                    return ::fmt::format("<![CDATA[{}]]>", pattern->getFormattedValue());
                });
            }

            void visit(ptrn::PatternPointer *pattern) override {
                processPattern(pattern, [this, pattern]() {
                    auto value = ::fmt::format("{:#x}", pattern->getValue());
                    m_output += ::fmt::format("{}<value>{}</value>\n", indent(m_depth + 1), value);
                    
                    if (pattern->getPointedAtPattern() != nullptr) {
                        m_output += ::fmt::format("{}<pointed_data>\n", indent(m_depth + 1));
                        
                        m_depth++;
                        pattern->getPointedAtPattern()->accept(*this);
                        m_depth--;
                        
                        m_output += ::fmt::format("{}</pointed_data>\n", indent(m_depth + 1));
                    }

                    return "";
                });
            }

            void visit(ptrn::PatternSigned *pattern) override {
                processPattern(pattern, [pattern]() {
                    return ::fmt::format("{}", pattern->getValue());
                });
            }

            void visit(ptrn::PatternString *pattern) override {
                processPattern(pattern, [pattern]() {
                    return ::fmt::format("{}", escapeXml(pattern->getValue()));
                });
            }

            void visit(ptrn::PatternStruct *pattern) override {
                processPattern(pattern, [this, pattern]() {
                    for (auto &[name, member] : pattern->getMembers()) {
                        m_output += ::fmt::format("{}<member name=\"{}\">\n", indent(m_depth + 1), escapeXml(name));
                        
                        m_depth++;
                        member->accept(*this);
                        m_depth--;
                        
                        m_output += ::fmt::format("{}</member>\n", indent(m_depth + 1));
                    }
                });
            }

            void visit(ptrn::PatternUnion *pattern) override {
                processPattern(pattern, [this, pattern]() {
                    for (auto &[name, member] : pattern->getMembers()) {
                        m_output += ::fmt::format("{}<member name=\"{}\">\n", indent(m_depth + 1), escapeXml(name));
                        
                        m_depth++;
                        member->accept(*this);
                        m_depth--;
                        
                        m_output += ::fmt::format("{}</member>\n", indent(m_depth + 1));
                    }
                });
            }

            void visit(ptrn::PatternUnsigned *pattern) override {
                processPattern(pattern, [pattern]() {
                    return ::fmt::format("{}", pattern->getValue());
                });
            }

            void visit(ptrn::PatternWideCharacter *pattern) override {
                processPattern(pattern, [pattern]() {
                    auto value = pattern->getValue();
                    if (value <= 0x7F && std::isprint(value))
                        return ::fmt::format("'{}'", escapeXml(std::string(1, value)));
                    else
                        return ::fmt::format("{:#x}", value);
                });
            }

            void visit(ptrn::PatternWideString *pattern) override {
                processPattern(pattern, [pattern]() {
                    return ::fmt::format("{}", escapeXml(pattern->getValue()));
                });
            }

            [[nodiscard]] std::string getOutput() const {
                return m_output;
            }

        private:
            const PatternLanguage &m_runtime;
            std::string m_output;
            u32 m_depth = 0;

            template<typename Func>
            void processPattern(ptrn::Pattern *pattern, Func func) {
                auto typeName = pattern->getTypeName();
                auto endian = pattern->getEndian() == std::endian::little ? "little" : "big";
                auto colorHex = ::fmt::format("#{:08X}", pattern->getColor());
                auto address = ::fmt::format("{:#x}", pattern->getOffset());
                auto size = ::fmt::format("{}", pattern->getSize());
                
                m_output += ::fmt::format("{}<{} name=\"{}\" endian=\"{}\" color=\"{}\" address=\"{}\" size=\"{}\"", 
                    indent(m_depth), 
                    typeName,
                    escapeXml(pattern->getVariableName()),
                    endian,
                    colorHex,
                    address,
                    size
                );

                auto comment = pattern->getComment();
                if (!comment.empty()) {
                    m_output += ::fmt::format(" comment=\"{}\"", escapeXml(comment));
                }

                auto result = func();
                if (result.empty()) {
                    if (typeName == "padding" || typeName == "union" || typeName == "struct" || 
                        typeName == "array_static" || typeName == "array_dynamic" || typeName == "bitfield" || 
                        typeName == "pointer") {
                        m_output += ">\n";
                        m_output += ::fmt::format("{}</{}>", indent(m_depth), typeName);
                    } else {
                        m_output += "/>";
                    }
                } else {
                    m_output += ">";
                    m_output += result;
                    m_output += ::fmt::format("</{}>", typeName);
                }
                m_output += "\n";
            }

            static std::string indent(u32 depth) {
                return std::string(depth * 2, ' ');
            }

            static std::string escapeXml(const std::string &str) {
                std::string result;
                result.reserve(str.size());
                
                for (char c : str) {
                    switch (c) {
                        case '&': result += "&amp;"; break;
                        case '<': result += "&lt;"; break;
                        case '>': result += "&gt;"; break;
                        case '"': result += "&quot;"; break;
                        case '\'': result += "&apos;"; break;
                        default: result += c; break;
                    }
                }
                
                return result;
            }
        };

        static std::string generateXml(const PatternLanguage &runtime) {
            XmlVisitor visitor(runtime);
            visitor.enableMetaInformation(true);
            
            auto &evaluator = runtime.getInternals().evaluator;
            auto patterns = runtime.getPatterns();
            
            std::string output = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            output += "<pattern_language>\n";
            
            // Add file metadata
            output += "  <metadata>\n";
            output += ::fmt::format("    <base_address>{:#x}</base_address>\n", evaluator->getDataBaseAddress());
            output += ::fmt::format("    <data_size>{}</data_size>\n", evaluator->getDataSize());
            output += "  </metadata>\n";
            
            // Add patterns
            output += "  <patterns>\n";
            
            for (auto &pattern : patterns) {
                if (pattern->isGlobalPattern()) {
                    visitor.m_depth = 2;
                    pattern->accept(visitor);
                }
            }
            
            output += "  </patterns>\n";
            output += "</pattern_language>";
            
            return output + visitor.getOutput();
        }
    };

}
