(function($) {
  'use strict';

  Drupal.behaviors.alma_item_list = {
    attach: function (context) {

      $('.alma-item-list', context).each(function () {
        var hash = $(this).data('hash');
        var alma_item_list = (Drupal.settings[hash] !== undefined) ? Drupal.settings[hash] : '';
        if (alma_item_list !== '') {
          $.getJSON(Drupal.settings.basePath + 'alma_item_list/' + hash, {content: alma_item_list}, function (content) {
            $('.pane-alma-item-list', context).find('[data-hash=' + hash + ']').replaceWith(content);
          });
        }
      });
    }
  };
} (jQuery));
