/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

//////////////////////////////////////////////////////////////////////////////
// PHTML

/** This class describes a HyperText markup Language string as used by the
   World Wide Web and the <code>PURL</code> and <code>PHTTP</code> class.
   
   All of the standard stream I/O operators, manipulators etc will operate on
   the PString class.
 */
class PHTML : PStringStream
{
  private:
	PHTML(const PHTML &);
	
  public:
    enum ElementInSet {
      InHTML,
      InHead,
      InBody,
      InTitle,
      InHeading,
      InDivision,
      InPreFormat,
      InAnchor,
      InNote,
      InAddress,
      InBlockQuote,
      InCredit,
      InBold,
      InItalic,
      InTeleType,
      InUnderline,
      InStrikeThrough,
      InBig,
      InSmall,
      InSubscript,
      InSuperscript,
      InEmphasis,
      InCite,
      InStrong,
      InCode,
      InSample,
      InKeyboard,
      InVariable,
      InDefinition,
      InQuote,
      InAuthor,
      InPerson,
      InAcronym,
      InAbbrev,
      InInsertedText,
      InDeletedText,
      InList,
      InListHeading,
      InDefinitionTerm,
      InTable,
      InForm,
      InSelect,
      InTextArea,
      NumElementsInSet
    };

    /** Construct a new HTML object. If a title is specified in the
       constructor then the HEAD, TITLE and BODY elements are output and the
       string is used in a H1 element.
     */
    PHTML(
      ElementInSet initialState = NumElementsInSet
    );
    PHTML(
      const char * cstr     ///< C string representation of the title string.
    );
    PHTML(
      const PString & str   ///< String representation of the title string.
    );

    ~PHTML();

	// xxx There is no assignment operator in Python xxx
    /** Restart the HTML string output using the specified value as the
       new title. If <CODE>title</CODE> is empty then no HEAD or TITLE
       elements are placed into the HTML.
     */
    // PHTML & operator=(
      // const PHTML & html     ///< HTML stream to make a copy of.
    // );
    // PHTML & operator=(
      // const PString & str    ///< String for title in restating HTML.
    // );
    // PHTML & operator=(
      // const char * cstr    ///< String for title in restating HTML.
    // );
    // PHTML & operator=(
      // char ch    ///< String for title in restating HTML.
    // );


  // New functions for class.
    PBoolean Is(ElementInSet elmt) const;
    void Set(ElementInSet elmt);
    void Clr(ElementInSet elmt);
    void Toggle(ElementInSet elmt);


    class Element {
      public: 
        virtual ~Element();
      protected:
        enum OptionalCRLF { NoCRLF, OpenCRLF, CloseCRLF, BothCRLF };
        Element(
          const char * nam,
          const char * att,
          PHTML::ElementInSet elmt,
          PHTML::ElementInSet req,
          OptionalCRLF opt
        );
        virtual void Output(PHTML & html) const;
        virtual void AddAttr(PHTML & html) const;
    };

    class HTML : PHTML::Element {
      public:
        HTML(const char * attr = NULL);
        virtual ~HTML();
    };

    class Head : PHTML::Element {
      public:
        Head();
        virtual ~Head();
      protected:
        virtual void Output(PHTML & html) const;
    };

    class Body : PHTML::Element {
      public:
        Body(const char * attr = NULL);
        virtual ~Body();
      protected:
        virtual void Output(PHTML & html) const;
    };

    class Title : PHTML::Element {
      public:
        Title();
        Title(const char * titleCStr);
        Title(const PString & titleStr);
        virtual ~Title();
      protected:
        virtual void Output(PHTML & html) const;
    };

    class Banner : PHTML::Element {
      public:
        Banner(const char * attr = NULL);
        virtual ~Banner();
    };

    class Division : PHTML::Element {
      public:
        Division(const char * attr = NULL);
        virtual ~Division();
    };

    class Heading : PHTML::Element {
      public:
        Heading(int number,
                int sequence = 0,
                int skip = 0,
                const char * attr = NULL);
        Heading(int number,
                const char * image,
                int sequence = 0,
                int skip = 0,
                const char * attr = NULL);
        Heading(int number,
                const PString & imageStr,
                int sequence = 0,
                int skip = 0,
                const char * attr = NULL);
        virtual ~Heading();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class BreakLine : PHTML::Element {
      public:
        BreakLine(const char * attr = NULL);
        virtual ~BreakLine();
    };

    class Paragraph : PHTML::Element {
      public:
        Paragraph(const char * attr = NULL);
        virtual ~Paragraph();
    };

    class PreFormat : PHTML::Element {
      public:
        PreFormat(int widthInChars = 0,
                  const char * attr = NULL);
        virtual ~PreFormat();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class HotLink : PHTML::Element {
      public:
        HotLink(const char * href = NULL, const char * attr = NULL);
        virtual ~HotLink();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class Target : PHTML::Element {
      public:
        Target(const char * name = NULL, const char * attr = NULL);
        virtual ~Target();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class ImageElement : PHTML::Element {
      protected:
        ImageElement(const char * nam,
                     const char * attr,
                     PHTML::ElementInSet elmt,
                     PHTML::ElementInSet req,
                     OptionalCRLF opt,
                     const char * image);
        virtual ~ImageElement();
        virtual void AddAttr(PHTML & html) const;
    };

    class Image : PHTML::ImageElement {
      public:
        Image(const char * src,
              int width = 0,
              int height = 0,
              const char * attr = NULL);
        Image(const char * src,
              const char * alt,
              int width = 0,
              int height = 0,
              const char * attr = NULL);
        virtual ~Image();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class HRule : PHTML::ImageElement {
      public:
        HRule(const char * image = NULL, const char * attr = NULL);
        virtual ~HRule();
    };

    class Note : PHTML::ImageElement {
      public:
        Note(const char * image = NULL, const char * attr = NULL);
        virtual ~Note();
    };

    class Address : PHTML::Element {
      public:
        Address(const char * attr = NULL);
        virtual ~Address();
    };

    class BlockQuote : PHTML::Element {
      public:
        BlockQuote(const char * attr = NULL);
        virtual ~BlockQuote();
    };

    class Credit : PHTML::Element {
      public:
        Credit(const char * attr = NULL);
        virtual ~Credit();
    };

    class SetTab : PHTML::Element {
      public:
        SetTab(const char * id, const char * attr = NULL);
        virtual ~SetTab();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class Tab : PHTML::Element {
      public:
        Tab(int indent, const char * attr = NULL);
        Tab(const char * id, const char * attr = NULL);
        virtual ~Tab();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };


    class Bold : PHTML::Element {
      public: Bold();
      virtual ~Bold();
    };
    class Italic : PHTML::Element {
      public: 
        Italic();
        virtual ~Italic();
    };
    class TeleType : PHTML::Element {
      public: 
        TeleType();
        virtual ~TeleType();
    };
    class Underline : PHTML::Element {
      public: 
        Underline();
        virtual ~Underline();
    };
    class StrikeThrough : PHTML::Element {
      public: 
        StrikeThrough();
        virtual ~StrikeThrough();
    };
    class Big : PHTML::Element {
      public: 
        Big();
        virtual ~Big();
    };
    class Small : PHTML::Element {
      public: 
        Small();
        virtual ~Small();
    };
    class Subscript : PHTML::Element {
      public: 
        Subscript();
        virtual ~Subscript();
    };
    class Superscript : PHTML::Element {
      public: 
        Superscript();
        virtual ~Superscript();
    };
    class Emphasis : PHTML::Element {
      public: 
        Emphasis();
        virtual ~Emphasis();
    };
    class Cite : PHTML::Element {
      public: 
        Cite();
      virtual ~Cite();
    };
    class Strong : PHTML::Element {
      public: 
        Strong();
        virtual ~Strong();
    };
    class Code : PHTML::Element {
      public: 
        Code();
      virtual ~Code();
    };
    class Sample : PHTML::Element {
      public: 
        Sample();
      virtual ~Sample();
    };
    class Keyboard : PHTML::Element {
      public: 
        Keyboard();
        virtual ~Keyboard();
    };
    class Variable : PHTML::Element {
      public: 
        Variable();
        virtual ~Variable();
    };
    class Definition : PHTML::Element {
      public: 
        Definition();
        virtual ~Definition();
    };
    class Quote : PHTML::Element {
      public: 
        Quote();
        virtual ~Quote();
    };
    class Author : PHTML::Element {
      public: 
        Author();
        virtual ~Author();
    };
    class Person : PHTML::Element {
      public: 
        Person();
        virtual ~Person();
    };
    class Acronym : PHTML::Element {
      public: 
        Acronym();
        virtual ~Acronym();
    };
    class Abbrev : PHTML::Element {
      public: 
        Abbrev();
        virtual ~Abbrev();
    };
    class InsertedText : PHTML::Element {
      public: 
        InsertedText();
        virtual ~InsertedText();
    };
    class DeletedText : PHTML::Element {
      public: 
        DeletedText();
      public: 
        virtual ~DeletedText();
    };

    class SimpleList : PHTML::Element {
      public:
        SimpleList(const char * attr = NULL);
        virtual ~SimpleList();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class BulletList : PHTML::Element {
      public:
        BulletList(const char * attr = NULL);
        virtual ~BulletList();
    };

    class OrderedList : PHTML::Element {
      public:
        OrderedList(int seqNum = 0, const char * attr = NULL);
        virtual ~OrderedList();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class DefinitionList : PHTML::Element {
      public:
        DefinitionList(const char * attr = NULL);
        virtual ~DefinitionList();
    };

    class ListHeading : PHTML::Element {
      public:
        ListHeading(const char * attr = NULL);
        virtual ~ListHeading();
    };

    class ListItem : PHTML::Element {
      public:
        ListItem(int skip = 0, const char * attr = NULL);
        virtual ~ListItem();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class DefinitionTerm : PHTML::Element {
      public:
        DefinitionTerm(const char * attr = NULL);
        virtual ~DefinitionTerm();
      protected:
        virtual void Output(PHTML & html) const;
    };

    class DefinitionItem : PHTML::Element {
      public:
        DefinitionItem(const char * attr = NULL);
        virtual ~DefinitionItem();
      protected:
        virtual void Output(PHTML & html) const;
    };


    enum BorderCodes {
      NoBorder,
      Border
    };
    class TableStart : PHTML::Element {
      public:
        TableStart(const char * attr = NULL);
        TableStart(PHTML::BorderCodes border, const char * attr = NULL);
        virtual ~TableStart();
      protected:
        virtual void Output(PHTML & html) const;
        virtual void AddAttr(PHTML & html) const;
    };

    class TableEnd : PHTML::Element {
      public:
        TableEnd();
        virtual ~TableEnd();
      protected:
        virtual void Output(PHTML & html) const;
    };


    class TableRow : PHTML::Element {
      public:
        TableRow(const char * attr = NULL);
        virtual ~TableRow();
    };

    class TableHeader : PHTML::Element {
      public:
        TableHeader(const char * attr = NULL);
        virtual ~TableHeader();
    };

    class TableData : PHTML::Element {
      public:
        TableData(const char * attr = NULL);
        virtual ~TableData();
    };


    class Form : PHTML::Element {
      public:
        Form(
          const char * method = NULL,
          const char * action = NULL,
          const char * encoding = NULL,
          const char * script = NULL
        );
        virtual ~Form();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    enum DisableCodes {
      Enabled,
      Disabled
    };
	
    class FieldElement : PHTML::Element {
      protected:
        FieldElement(
          const char * nam,
          const char * attr,
          PHTML::ElementInSet elmt,
          OptionalCRLF opt,
          PHTML::DisableCodes disabled
        );
        virtual ~FieldElement();
        virtual void AddAttr(PHTML & html) const;
    };

    class Select : PHTML::FieldElement {
      public:
        Select(
          const char * fname = NULL,
          const char * attr = NULL
        );
        Select(
          const char * fname,
          PHTML::DisableCodes disabled,
          const char * attr = NULL
        );
        virtual ~Select();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    enum SelectionCodes {
      NotSelected,
      Selected
    };
    class Option : PHTML::FieldElement {
      public:
        Option(
          const char * attr = NULL
        );
        Option(
          PHTML::SelectionCodes select,
          const char * attr = NULL
        );
        Option(
          PHTML::DisableCodes disabled,
          const char * attr = NULL
        );
        Option(
          PHTML::SelectionCodes select,
          PHTML::DisableCodes disabled,
          const char * attr = NULL
        );
        virtual ~Option();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class FormField : PHTML::FieldElement {
      protected:
        FormField(
          const char * nam,
          const char * attr,
          PHTML::ElementInSet elmt,
          OptionalCRLF opt,
          PHTML::DisableCodes disabled,
          const char * fname
        );
        virtual ~FormField();
        virtual void AddAttr(PHTML & html) const;
    };

    class TextArea : PHTML::FormField {
      public:
        TextArea(
          const char * fname,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        TextArea(
          const char * fname,
          int rows, int cols,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~TextArea();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class InputField : PHTML::FormField {
      protected:
        InputField(
          const char * type,
          const char * fname,
          PHTML::DisableCodes disabled,
          const char * attr
        );
        virtual ~InputField();
        virtual void AddAttr(PHTML & html) const;
    };

    class HiddenField : PHTML::InputField {
      public:
        HiddenField(
          const char * fname,
          const char * value,
          const char * attr = NULL
        );
        virtual ~HiddenField();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class InputText : PHTML::InputField {
      public:
        InputText(
          const char * fname,
          int size,
          const char * init = NULL,
          const char * attr = NULL
        );
        // InputText(
          // const char * fname,
          // int size,
          // PHTML::DisableCodes disabled,
          // const char * attr = NULL
        // );
        InputText(
          const char * fname,
          int size,
          int maxLength,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        InputText(
          const char * fname,
          int size,
          const char * init,
          int maxLength,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~InputText();
      protected:
        InputText(
          const char * type,
          const char * fname,
          int size,
          const char * init,
          int maxLength,
          PHTML::DisableCodes disabled,
          const char * attr
        );
        virtual void AddAttr(PHTML & html) const;
    };

    class InputPassword : PHTML::InputText {
      public:
        InputPassword(
          const char * fname,
          int size,
          const char * init = NULL,
          const char * attr = NULL
        );
        // InputPassword(
          // const char * fname,
          // int size,
          // PHTML::DisableCodes disabled,
          // const char * attr = NULL
        // );
        InputPassword(
          const char * fname,
          int size,
          int maxLength,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        InputPassword(
          const char * fname,
          int size,
          const char * init,
          int maxLength,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~InputPassword();
    };

    enum CheckedCodes {
      UnChecked,
      Checked
    };
    class RadioButton : PHTML::InputField {
      public:
        RadioButton(
          const char * fname,
          const char * value,
          const char * attr = NULL
        );
        RadioButton(
          const char * fname,
          const char * value,
          PHTML::DisableCodes disabled,
          const char * attr = NULL
        );
        RadioButton(
          const char * fname,
          const char * value,
          PHTML::CheckedCodes check,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~RadioButton();
      protected:
        RadioButton(
          const char * type,
          const char * fname,
          const char * value,
          PHTML::CheckedCodes check,
          PHTML::DisableCodes disabled,
          const char * attr
        );
        virtual void AddAttr(PHTML & html) const;
    };

    class CheckBox : PHTML::RadioButton {
      public:
        CheckBox(
          const char * fname,
          const char * attr = NULL
        );
        CheckBox(
          const char * fname,
          PHTML::DisableCodes disabled,
          const char * attr = NULL
        );
        CheckBox(
          const char * fname,
          PHTML::CheckedCodes check,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~CheckBox();
    };


    class InputNumber : PHTML::InputField {
      public:
        InputNumber(
          const char * fname,
          int min, int max,
          int value = 0,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~InputNumber();
      protected:
        InputNumber(
          const char * type,
          const char * fname,
          int min, int max,
          int value,
          PHTML::DisableCodes disabled,
          const char * attr
        );
        virtual void AddAttr(PHTML & html) const;
    };

    class InputRange : PHTML::InputNumber {
      public:
        InputRange(
          const char * fname,
          int min, int max,
          int value = 0,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~InputRange();
    };

    class InputFile : PHTML::InputField {
      public:
        InputFile(
          const char * fname,
          const char * accept = NULL,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~InputFile();
      protected:
        virtual void AddAttr(PHTML & html) const;
    };

    class InputImage : PHTML::InputField {
      public:
        InputImage(
          const char * fname,
          const char * src = NULL,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~InputImage();
      protected:
        InputImage(
          const char * type,
          const char * fname,
          const char * src,
          PHTML::DisableCodes disabled,
          const char * attr
        );
        virtual void AddAttr(PHTML & html) const;
    };

    class InputScribble : PHTML::InputImage {
      public:
        InputScribble(
          const char * fname,
          const char * src = NULL,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~InputScribble();
    };

    class ResetButton : PHTML::InputImage {
      public:
        ResetButton(
          const char * title,
          const char * fname = NULL,
          const char * src = NULL,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~ResetButton();
      protected:
        ResetButton(
          const char * type,
          const char * title,
          const char * fname = NULL,
          const char * src = NULL,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual void AddAttr(PHTML & html) const;
    };

    class SubmitButton : PHTML::ResetButton {
      public:
        SubmitButton(
          const char * title,
          const char * fname = NULL,
          const char * src = NULL,
          PHTML::DisableCodes disabled = PHTML::Enabled,
          const char * attr = NULL
        );
        virtual ~SubmitButton();
    };


  protected:
    virtual void AssignContents(const PContainer & c);
};




// End Of File ///////////////////////////////////////////////////////////////
