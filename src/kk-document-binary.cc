//
//  kk-document-binary.cc
//  KKElement
//
//  Created by zhanghailong on 2018/8/7.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//

#include "kk-config.h"
#include "kk-document-binary.h"
#include "kk-element.h"

#if defined(KK_PLATFORM_IOS)

#include <KKObject/kk-bio.h>

#else

#include "kk-bio.h"

#endif

#define BUF_EX_SIZE 40960

//#define KK_DEBUG_SYNC(...) kk::Log(__VA_ARGS__)

#define KK_DEBUG_SYNC(...)

namespace kk {
    
    static char TAG[] = {'K','K',0x00,0x00};
    
    IMP_SCRIPT_CLASS_BEGIN(nullptr, DocumentBinaryObserver, DocumentBinaryObserver)
    
    static kk::script::Method methods[] = {
        {"data",(kk::script::Function) &DocumentBinaryObserver::duk_data},
        {"encode",(kk::script::Function) &DocumentBinaryObserver::duk_encode},
    };
    
    kk::script::SetMethod(ctx, -1, methods, sizeof(methods) / sizeof(kk::script::Method));
    
    static kk::script::Property propertys[] = {
        {"title",(kk::script::Function) &DocumentBinaryObserver::duk_title,(kk::script::Function) &DocumentBinaryObserver::duk_setTitle},
    };
    
    kk::script::SetProperty(ctx, -1, propertys, sizeof(propertys) / sizeof(kk::script::Property));
    
    duk_push_c_function(ctx, DocumentBinaryObserver::duk_decode, 3);
    duk_put_prop_string(ctx, -2, "decode");
    
    IMP_SCRIPT_CLASS_END
    
    
    DocumentBinaryObserver::DocumentBinaryObserver():_data(nullptr),_size(0),_length(0) {
        
    }
    
    DocumentBinaryObserver::~DocumentBinaryObserver() {
        if(_data) {
            free(_data);
        }
    }
    
    void DocumentBinaryObserver::alloc(Document * document,Element * element) {
        append((Byte)DocumentObserverTypeAlloc);
        append((Int64)element->elementId());
        append((CString)element->name());
    }
    
    void DocumentBinaryObserver::root(Document * document,Element * element) {
        append((Byte)DocumentObserverTypeRoot);
        if(element == nullptr ){
            append((Int64) 0);
        } else {
            append((Int64) element->elementId());
        }
    }
    
    void DocumentBinaryObserver::set(Document * document,Element * element,ElementKey key,CString value) {
        append((Byte)DocumentObserverTypeSet);
        append((Int64)element->elementId());
        append((Int64)key);
        append((CString)value);
    }
    
    void DocumentBinaryObserver::append(Document * document, Element * element,Element * e) {
        append((Byte)DocumentObserverTypeAppend);
        append((Int64)element->elementId());
        append((Int64)e->elementId());
    }
    
    void DocumentBinaryObserver::before(Document * document, Element * element,Element * e) {
        append((Byte)DocumentObserverTypeBefore);
        append((Int64)element->elementId());
        append((Int64)e->elementId());
    }
    
    void DocumentBinaryObserver::after(Document * document, Element * element,Element * e) {
        append((Byte)DocumentObserverTypeAfter);
        append((Int64)element->elementId());
        append((Int64)e->elementId());
    }
    
    void DocumentBinaryObserver::remove(Document * document, ElementKey elementId) {
        append((Byte)DocumentObserverTypeRemove);
        append((Int64)elementId);
    }
    
    void DocumentBinaryObserver::key(Document * document, ElementKey key, CString name) {
        append((Byte)DocumentObserverTypeKey);
        append((Int64)key);
        append((CString)name);
    }
    
    static void encodeAttributes(DocumentBinaryObserver * observer,Document * document, Element * element) {
    
        std::map<String,String> & attrs = element->attributes();
        std::map<String,String>::iterator i = attrs.begin();
        
        while(i != attrs.end()) {
            observer->set(document, element, document->elementKey(i->first.c_str()), i->second.c_str());
            i ++;
        }
        
    }
    
    static void encodeElement(DocumentBinaryObserver * observer,Document * document, Element * element) {

        encodeAttributes(observer,document,element);
        
        Element * p = element->firstChild();
        
        while(p) {
            observer->alloc(document, p);
            observer->append(document, element, p);
            encodeElement(observer, document, p);
            p = p->nextSibling();
        }
        
    }
    
    void DocumentBinaryObserver::encode(Document * document) {
        
        {
            std::map<ElementKey,String> & keys = document->elementKeys();
            std::map<ElementKey,String>::iterator i = keys.begin();
            while(i != keys.end()) {
                this->key(document, i->first, i->second.c_str());
                i ++;
            }
        }
        
        {
            Element * p = document->rootElement();
            
            if(p != nullptr) {
                this->alloc(document, p);
                encodeElement(this,document,p);
            }
            
            this->root(document, p);
            
        }
        
    }
    
    Byte * DocumentBinaryObserver::data() {
        return _data;
    }
    
    size_t DocumentBinaryObserver::length() {
        return _length;
    }
    
    kk::CString DocumentBinaryObserver::title() {
        return _title.c_str();
    }
    
    void DocumentBinaryObserver::setTitle(kk::CString title) {
        _title = title == nullptr ? "" : title;
    }
    
    duk_ret_t DocumentBinaryObserver::duk_setTitle(duk_context * ctx) {
        
        int top = duk_get_top(ctx);
        
        if(top >0 && duk_is_string(ctx, -top)) {
            setTitle(duk_to_string(ctx, -top));
        }
        
        return 0;
    }
    
    duk_ret_t DocumentBinaryObserver::duk_title(duk_context * ctx) {
        duk_push_string(ctx, _title.c_str());
        return 1;
    }
    
    void DocumentBinaryObserver::append(Byte * data, size_t n) {
       
        presize(n);
        
        memcpy(_data + _length, data, n);
        
        _length += n;
        
    }
    
    void DocumentBinaryObserver::append(Byte byte) {
        append(&byte, 1);
    }
    
    void DocumentBinaryObserver::append(Int32 v) {
        presize(Bio::Int32_Size);
        _length += Bio::encode(v, _data + _length, _size - _length);
    }
    
    void DocumentBinaryObserver::append(Int64 v) {
        presize(Bio::Int64_Size);
        _length += Bio::encode(v, _data + _length, _size - _length);
    }
    
    void DocumentBinaryObserver::append(Boolean v) {
        presize(Bio::Boolean_Size);
        _length += Bio::encode(v, _data + _length, _size - _length);
    }
    
    void DocumentBinaryObserver::append(Float v) {
        presize(Bio::Float_Size);
        _length += Bio::encode(v, _data + _length, _size - _length);
    }
    
    void DocumentBinaryObserver::append(Double v) {
        presize(Bio::Double_Size);
        _length += Bio::encode(v, _data + _length, _size - _length);
    }
    
    void DocumentBinaryObserver::append(CString v) {
        
        size_t n = v == nullptr ? 0 : strlen(v) + 1;
        
        presize(Bio::Int32_Size + n);
        
        _length += Bio::encode((Int32) n, _data + _length, _size - _length);
        
        if(v != nullptr) {
            append((Byte *) v, n);
        }
    
    }
    
    void DocumentBinaryObserver::presize(size_t length) {
        if(_length + length > _size) {
            _size = MAX(sizeof(TAG) + _length + length,_size +BUF_EX_SIZE);
            if(_data == nullptr) {
                _data = (Byte *) malloc(_size);
                memcpy(_data, TAG, sizeof(TAG));
                _length = sizeof(TAG);
            } else {
                _data = (Byte *) realloc(_data, _size);
            }
        }
    }
    
    size_t DocumentBinaryObserver::decode(Document * document,Byte * data, size_t size,kk::CString title) {
        
        KK_DEBUG_SYNC("[SYNC] >>>>>>>>>>");
        
        std::map<ElementKey,Strong> elements;
    
        size_t n = 0;
        
        if(size >= sizeof(TAG)) {
            
            if(memcmp(data, TAG, sizeof(TAG)) != 0) {
                KK_DEBUG_SYNC("[DOCUMENT] [BINARY] [DECODE] [ERROR] TAG");
                return 0;
            }
            
            n += sizeof(TAG);
        }
        
        while(n < size) {
            
            Byte v = 0;
            
            n += Bio::decode(&v, data + n, size - n);
            
            if(v == DocumentObserverTypeAlloc ) {
                
                if(size - n >= Bio::Int64_Size + Bio::Int32_Size) {
                    
                    Int64 elementId = 0;
                    Int32 length = 0;
                    n += Bio::decode(& elementId, data + n, size - n);
                    n += Bio::decode(& length, data + n, size - n);
                    
                    if(elementId && length > 0) {
                        Strong v = document->element(elementId);
                        if(v.get() == nullptr) {
                            Strong vv = document->createElement(data + n, elementId);
                            elements[elementId] = vv.get();
                            KK_DEBUG_SYNC("[SYNC] [ALLOC] %lld %s",elementId,data + n);
                        }
                    } else {
                        KK_DEBUG_SYNC("[DOCUMENT] [BINARY] [DECODE] [ERROR] [ALLOC]");
                    }
                    
                    n += length;
                    
                } else {
                    break;
                }
            } else if(v == DocumentObserverTypeRoot) {
                
                if(size - n >= Bio::Int64_Size) {
                    
                    Int64 elementId = 0;
                    
                    n += Bio::decode(& elementId, data + n, size - n);
                    
                    Strong e = document->element(elementId);
     
                    document->setRootElement((Element *) e.get());
                    
                    KK_DEBUG_SYNC("[SYNC] [ROOT] %lld",elementId);
                    
                } else {
                    break;
                }
                
            } else if(v == DocumentObserverTypeSet) {
                
                if(size - n >= Bio::Int64_Size + Bio::Int64_Size + Bio::Int32_Size) {
                    
                    Int64 elementId = 0;
                    Int64 key = 0;
                    Int32 length = 0;
                    
                    n += Bio::decode(& elementId, data + n, size - n);
                    n += Bio::decode(& key, data + n, size - n);
                    n += Bio::decode(& length, data + n, size - n);
                    
                    Strong e = document->element(elementId);
                    Element * element = (Element *) e.get();
                    
                    if(element != nullptr) {
                    
                        CString skey = document->key(key);
                        
                        if(skey != nullptr) {
                            element->set(skey, length == 0 ? nullptr : data + n);
                            KK_DEBUG_SYNC("[SYNC] [SET] %lld %s=%s",elementId,skey, length == 0 ? "" : data + n);
                        }
                        
                    } else {
                        KK_DEBUG_SYNC("[DOCUMENT] [BINARY] [DECODE] [ERROR] [SET]");
                    }
                    
                    n += length;
                    
                } else {
                    break;
                }
            } else if(v == DocumentObserverTypeAppend) {
                
                if(size - n >= Bio::Int64_Size + Bio::Int64_Size ) {
                    
                    Int64 elementId = 0;
                    Int64 eid = 0;
                    
                    n += Bio::decode(& elementId, data + n, size - n);
                    n += Bio::decode(& eid, data + n, size - n);
                    
                    Strong e = document->element(elementId);
                    Element * element = (Element *) e.get();
                    
                    if(element != nullptr) {
                        
                        Strong ee = document->element(eid);
                        Element * el = (Element *) ee.get();
                        
                        if(el != nullptr && el->parent() != element) {
                            element->append(el);
                            KK_DEBUG_SYNC("[SYNC] [APPEND] %lld %lld",elementId,eid);
                        }
                        
                    } else {
                        KK_DEBUG_SYNC("[DOCUMENT] [BINARY] [DECODE] [ERROR] [APPEND] %lld %lld",elementId,eid);
                    }
                    
                } else {
                    break;
                }
            } else if(v == DocumentObserverTypeBefore) {
                
                if(size - n >= Bio::Int64_Size + Bio::Int64_Size ) {
                    
                    Int64 elementId = 0;
                    Int64 eid = 0;
                    
                    n += Bio::decode(& elementId, data + n, size - n);
                    n += Bio::decode(& eid, data + n, size - n);
                    
                    Strong e = document->element(elementId);
                    Element * element = (Element *) e.get();
                    
                    if(element != nullptr) {
                        
                        Strong ee = document->element(eid);
                        Element * el = (Element *) ee.get();
                        
                        if(el != nullptr && el->nextSibling() != element) {
                            element->before(el);
                        }
                        
                    } else {
                        KK_DEBUG_SYNC("[DOCUMENT] [BINARY] [DECODE] [ERROR] [BEFORE]");
                    }
                    
                } else {
                    break;
                }
            } else if(v == DocumentObserverTypeAfter) {
                
                if(size - n >= Bio::Int64_Size + Bio::Int64_Size ) {
                    
                    Int64 elementId = 0;
                    Int64 eid = 0;
                    
                    n += Bio::decode(& elementId, data + n, size - n);
                    n += Bio::decode(& eid, data + n, size - n);
                    
                    Strong e = document->element(elementId);
                    Element * element = (Element *) e.get();
                    
                    if(element != nullptr) {
                        
                        Strong ee = document->element(eid);
                        Element * el = (Element *) ee.get();
                        
                        if(el != nullptr && el->prevSibling() != element) {
                            element->after(el);
                        }
                        
                    } else {
                        KK_DEBUG_SYNC("[DOCUMENT] [BINARY] [DECODE] [ERROR] [AFTER]");
                    }
                    
                } else {
                    break;
                }
            } else if(v == DocumentObserverTypeRemove) {
                
                if(size - n >= Bio::Int64_Size) {
                    
                    Int64 elementId = 0;
                    
                    n += Bio::decode(& elementId, data + n, size - n);
                    
                    Strong e = document->element(elementId);
                    Element * element = (Element *) e.get();
                    
                    if(element != nullptr) {
                        
                        elements[elementId] = element;
                        
                        element->remove();
                        
                    } else {
                        KK_DEBUG_SYNC("[DOCUMENT] [BINARY] [DECODE] [ERROR] [REMOVE]");
                    }
                    
                } else {
                    break;
                }
            } else if(v == DocumentObserverTypeKey) {
                
                if(size - n >= Bio::Int64_Size + Bio::Int32_Size) {
                    
                    Int64 key = 0;
                    Int32 length = 0;
                    n += Bio::decode(& key, data + n, size - n);
                    n += Bio::decode(& length, data + n, size - n);
                    
                    if(key && length > 0) {
                        document->set((CString) (data + n), key);
                    } else {
                        KK_DEBUG_SYNC("[DOCUMENT] [BINARY] [DECODE] [ERROR] [KEY]");
                    }
                    
                    n += length;
                    
                } else {
                    break;
                }
                
            } else {

                KK_DEBUG_SYNC("[DOCUMENT] [BINARY] [DECODE] [ERROR] [TYPE]");

            }
            
            
        }
        
        KK_DEBUG_SYNC("[SYNC] <<<<<<<<");
        
        return n;
    }
    
    
    duk_ret_t DocumentBinaryObserver::duk_data(duk_context * ctx) {
        
        if(_length > 0) {
            
            void * d = duk_push_fixed_buffer(ctx, _length);
    
            memcpy(d, _data, _length);
            
            duk_push_buffer_object(ctx, -1, 0, _length, DUK_BUFOBJ_UINT8ARRAY);
//
            duk_remove(ctx, -2);
            
            return 1;
            
        }
        
        return 0;
    }
    
    duk_ret_t DocumentBinaryObserver::duk_decode(duk_context * ctx) {
        
        int top = duk_get_top(ctx);
        
        if(top > 1 && duk_is_object(ctx, -top) && duk_is_buffer_data(ctx, - top + 1)) {
            
            kk::Object * v = kk::script::GetObject(ctx, -top);
            
            if(v) {
                
                Document * doc = dynamic_cast<Document *>(v);
                
                if(doc) {
                    
                    duk_size_t n;
                    Byte * bytes = (Byte *) duk_get_buffer_data(ctx, - top + 1, &n);
                    kk::CString title = nullptr;
                    
                    if(top > 2 && duk_is_string(ctx, - top + 2)) {
                        title = duk_to_string(ctx, -top + 2);
                    }
                    
                    DocumentBinaryObserver::decode(doc, bytes, n,title);
                    
                }
                
            }
        }
        
        return 0;
        
    }
    
    duk_ret_t DocumentBinaryObserver::duk_encode(duk_context * ctx) {
        
        int top = duk_get_top(ctx);
        
        if(top > 0 && duk_is_object(ctx, -top) ) {
            
            kk::Object * v = kk::script::GetObject(ctx, -top);
            
            if(v) {
                
                Document * doc = dynamic_cast<Document *>(v);
                
                if(doc) {
                    
                    encode(doc);
                    
                }
                
            }
        }
        
        return 0;
        
    }
    
    
}
