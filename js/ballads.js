if (!String.prototype.endsWith) {
  String.prototype.endsWith = function(searchString, position) {
      var subjectString = this.toString();
      if (typeof position !== 'number' || !isFinite(position) || Math.floor(position) !== position || position > subjectString.length) {
        position = subjectString.length;
      }
      position -= searchString.length;
      var lastIndex = subjectString.indexOf(searchString, position);
      return lastIndex !== -1 && lastIndex === position;
  };
}

(function($){

    var wordRx = /^([a-zA-Z][a-zA-Z0-9-]*)/;
    var spaceRx = /\s+/;
    var unitsRx = /(\d+(?:\.\d+)?)mm/;

    var minFontSize = 10;
    var mm2px = 4;

    function measurement(descriptor, isFont) {
        var mm = descriptor.match(unitsRx);
        if( ! mm) {
            return;
        }
        if(isFont) {
            return Math.max(minFontSize, mm[1] * mm2px) + 'px';
        }
        return (mm[1] * mm2px) + 'px';
    }

    function desc2style(descriptor, $element) {
        var word = descriptor.match(wordRx);
        switch(word[1]) {
            case 'center':
            case 'centre':
                $element.css('text-align', 'center');
                break;
            case 'italic':
            case 'italics':
                $element.css('font-style', 'italic');
                break;
            case 'roman':
                $element.css('font-style', 'normal');
            case 'uppercase':
                $element.css('text-transform', 'uppercase');
                break;
            case 'font-size':
                $element.css('font-size', measurement(descriptor, true));
                break;
            case 'indent':
                $element.css('margin-left', measurement(descriptor, false));
            case 'length':
                $element.css('width', measurement(descriptor, false));
                break;
            default:
                console.log('Unknown rendering descriptor "' + word + '"');
        }
    }
    
    function fixStyles() {
        $('[data-rend]').each(function() {
            var $this = $(this);
            var rend = $this.data('rend');
            var descriptors = rend.split(spaceRx);
            descriptors.map(function(desc) {desc2style(desc, $this);});
        });
    }
    
    function fixImageUrls() {
        $('img.facs').each(function() {
            var $this = $(this);
            $this.attr('src', window.location.pathname + '/' + $this.attr('src'));
        });
    }

    $(document).ready(function(){        
        fixStyles();
        if( ! window.location.pathname.endsWith('/')) {
            fixImageUrls();
        }
    });
    
})(jQuery);